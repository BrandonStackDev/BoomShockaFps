#include "map_parser.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_BRUSHES 1024
#define MAX_PLANES 32
#define MAX_VERTS_PER_FACE 32
#define MAX_TRIANGLES 1024
#define EPSILON_INTERNAL 0.01f
#define QUAKE_TO_METERS 0.0254f  // 1 inch = 0.0254 meters

typedef struct {
    Vector3 normal;
    float d;

    // UV mapping data
    char textureName[64];
    Vector3 texU, texV;
    float offsetU, offsetV;
    float scaleU, scaleV;
} Plane;

typedef struct {
    Plane planes[MAX_PLANES];
    int planeCount;
    char className[64];
    bool hasOrigin;
    Vector3 origin;
    bool hasSubType;
    char subType[64];
} Brush;

// -----------------------------
// Math Helpers
// -----------------------------

static Vector3 ConvertFromQuake(Vector3 v)
{
    // TrenchBroom: X right, Y forward, Z up
    // Raylib:      X right, Y up,     Z forward
    // So we convert: (x, y, z) -> (x, z, -y)
    return (Vector3){
        v.x,
        v.z,
        -v.y
    };
}

Vector3 ComputeFaceOrigin(Vector3 *faceVerts, int count) {
    Vector3 sum = { 0 };
    for (int i = 0; i < count; i++) {
        sum = Vector3Add(sum, faceVerts[i]);
    }
    return Vector3Scale(sum, 1.0f / count);
}

Vector3 RotateVectorAroundAxis(Vector3 v, Vector3 axis, float angleDeg)
{
    float angleRad = angleDeg * (PI / 180.0f);
    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);

    // Rodrigues' rotation formula
    return Vector3Add(
        Vector3Scale(v, cosA),
        Vector3Add(
            Vector3Scale(Vector3CrossProduct(axis, v), sinA),
            Vector3Scale(axis, Vector3DotProduct(axis, v) * (1.0f - cosA))
        )
    );
}

static Vector3 Cross(Vector3 a, Vector3 b) {
    return Vector3CrossProduct(a, b);
}

static float Dot(Vector3 a, Vector3 b) {
    return Vector3DotProduct(a, b);
}

static Vector3 NormalizeInternal(Vector3 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len < EPSILON_INTERNAL) return (Vector3){0};
    return (Vector3){v.x / len, v.y / len, v.z / len};
}

Plane PlaneFromPoints(Vector3 a, Vector3 b, Vector3 c)
{
    // a = Vector3Scale(ConvertFromQuake(a), QUAKE_TO_METERS);
    // b = Vector3Scale(ConvertFromQuake(b), QUAKE_TO_METERS);
    // c = Vector3Scale(ConvertFromQuake(c), QUAKE_TO_METERS);

    Vector3 normal = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
    normal = NormalizeInternal(normal);
    float d = Vector3DotProduct(normal, a);
    return (Plane){ normal, d };
}


int IntersectPlanes(Plane a, Plane b, Plane c, Vector3 *out)
{
    Vector3 n1 = a.normal;
    Vector3 n2 = b.normal;
    Vector3 n3 = c.normal;

    float denom = Vector3DotProduct(n1, Vector3CrossProduct(n2, n3));
    if (fabsf(denom) < EPSILON_INTERNAL) return 0;

    Vector3 term1 = Vector3Scale(Vector3CrossProduct(n2, n3), a.d);
    Vector3 term2 = Vector3Scale(Vector3CrossProduct(n3, n1), b.d);
    Vector3 term3 = Vector3Scale(Vector3CrossProduct(n1, n2), c.d);

    *out = Vector3Scale(Vector3Add(Vector3Add(term1, term2), term3), 1.0f / denom);
    return 1;
}


static int PointInsideBrush(Vector3 p, Brush *brush) {
    for (int i = 0; i < brush->planeCount; i++) {
        Plane pl = brush->planes[i];
        if (Dot(pl.normal, p) < pl.d - EPSILON_INTERNAL) 
        {
            printf("Outside plane %d: dot=%f, d=%f, epsilon=%f\n", i, Dot(pl.normal, p), pl.d, EPSILON_INTERNAL);
            return false;  // < not >
        }
    }
    return true;
}

static int SamePoint(Vector3 a, Vector3 b) {
    return Vector3Length(Vector3Subtract(a, b)) < EPSILON_INTERNAL;
}

// -----------------------------
// Sorting points for triangulation
// -----------------------------

static Vector3 ComputeFaceCenter(Vector3 *points, int count) {
    Vector3 center = {0};
    for (int i = 0; i < count; i++) center = Vector3Add(center, points[i]);
    return Vector3Scale(center, 1.0f / (float)count);
}

static void SortPointsOnPlane(Vector3 *points, int count, Plane plane)
{
    if (count < 3) return;

    Vector3 center = ComputeFaceCenter(points, count);

    // Build a local coordinate system on the face plane
    Vector3 normal = plane.normal;
    Vector3 ref = {1, 0, 0};  // arbitrary vector to generate axis

    if (fabsf(Vector3DotProduct(normal, ref)) > 0.9f) {
        ref = (Vector3){0, 0, 1};  // fallback if colinear
    }

    Vector3 axisX = Vector3Normalize(Vector3CrossProduct(normal, ref));
    Vector3 axisY = Vector3CrossProduct(normal, axisX);

    float angles[MAX_VERTS_PER_FACE];
    for (int i = 0; i < count; i++) {
        Vector3 rel = Vector3Subtract(points[i], center);
        float x = Vector3DotProduct(rel, axisX);
        float y = Vector3DotProduct(rel, axisY);
        angles[i] = atan2f(y, x);
    }

    // Simple insertion sort on angles
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (angles[i] > angles[j]) {
                float tempAngle = angles[i]; angles[i] = angles[j]; angles[j] = tempAngle;
                Vector3 temp = points[i]; points[i] = points[j]; points[j] = temp;
            }
        }
    }
}


// -----------------------------
// Brush → Mesh
// -----------------------------

static Mesh BuildMeshFromBrush(Brush *brush) {
    Vector3 verts[MAX_TRIANGLES * 3];
    int vertCount = 0;

    Vector3 intersectionPoints[256];
    int intersectionCount = 0;

    // Collect all valid triple-plane intersection points
    for (int i = 0; i < brush->planeCount; i++) {
        for (int j = i + 1; j < brush->planeCount; j++) {
            for (int k = j + 1; k < brush->planeCount; k++) {
                Vector3 p;
                if (IntersectPlanes(brush->planes[i], brush->planes[j], brush->planes[k], &p)) {
                    //TraceLog(LOG_INFO, "Triple-plane intersect at (%f, %f, %f)", p.x, p.y, p.z);
                    if (PointInsideBrush(p, brush)) {
                        //TraceLog(LOG_INFO, "  -> Point is inside brush!");
                        // Check for duplicates
                        int exists = 0;
                        for (int m = 0; m < intersectionCount; m++) {
                            if (SamePoint(p, intersectionPoints[m])) {
                                exists = 1; break;
                            }
                        }
                        if (!exists) intersectionPoints[intersectionCount++] = p;
                    } else {TraceLog(LOG_INFO, "  -> Point is outside brush!");}
                }
            }
        }
    }
    TraceLog(LOG_INFO, "Brush: %d planes, %d intersection points", brush->planeCount, intersectionCount);
    // For each plane, gather face verts
    int vertexFaceIndex[MAX_TRIANGLES * 3]; // one per vertex
    int triangleCount = 0;
    for (int i = 0; i < brush->planeCount; i++) {
        Vector3 faceVerts[MAX_VERTS_PER_FACE];
        int faceCount = 0;

        for (int j = 0; j < intersectionCount; j++) {
            float dist = Dot(brush->planes[i].normal, intersectionPoints[j]);
            if (fabsf(dist - brush->planes[i].d) < EPSILON_INTERNAL) {
                faceVerts[faceCount++] = intersectionPoints[j];
            }
        }

        if (faceCount < 3)
        {
            TraceLog(LOG_INFO, "Facecount low: %d ", faceCount);
            continue;
        }

        SortPointsOnPlane(faceVerts, faceCount, brush->planes[i]);

        Vector3 A0 = Vector3Scale(ConvertFromQuake(faceVerts[0]), QUAKE_TO_METERS);
        Vector3 B0 = Vector3Scale(ConvertFromQuake(faceVerts[1]), QUAKE_TO_METERS);
        Vector3 C0 = Vector3Scale(ConvertFromQuake(faceVerts[2]), QUAKE_TO_METERS);
        Vector3 baseNormal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(B0, A0), Vector3Subtract(C0, A0)));

        for (int v = 1; v < faceCount - 1; v++) {
            Vector3 A = Vector3Scale(ConvertFromQuake(faceVerts[0]), QUAKE_TO_METERS);
            Vector3 B = Vector3Scale(ConvertFromQuake(faceVerts[v]), QUAKE_TO_METERS);
            Vector3 C = Vector3Scale(ConvertFromQuake(faceVerts[v + 1]), QUAKE_TO_METERS);

            Vector3 triNormal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(B, A), Vector3Subtract(C, A)));

            if (Vector3DotProduct(triNormal, baseNormal) >= 0) {
                verts[vertCount] = A; vertexFaceIndex[vertCount++] = i;
                verts[vertCount] = C; vertexFaceIndex[vertCount++] = i;
                verts[vertCount] = B; vertexFaceIndex[vertCount++] = i;
            } else {
                verts[vertCount] = A; vertexFaceIndex[vertCount++] = i;
                verts[vertCount] = B; vertexFaceIndex[vertCount++] = i;
                verts[vertCount] = C; vertexFaceIndex[vertCount++] = i;
            }
        }

        
        //TraceLog(LOG_INFO, "Triangulated face with %d verts into %d triangles", faceCount, faceCount - 2);
    }

    Mesh mesh = {0};
    mesh.vertexCount = vertCount;
    mesh.triangleCount = vertCount / 3;

    mesh.vertices = MemAlloc(vertCount * 3 * sizeof(float));
    mesh.normals = MemAlloc(vertCount * 3 * sizeof(float));
    mesh.texcoords = MemAlloc(vertCount * 2 * sizeof(float));

    for (int i = 0; i < vertCount; i += 3) {
        Vector3 a = verts[i];
        Vector3 b = verts[i + 1];
        Vector3 c = verts[i + 2];
        Vector3 normal = NormalizeInternal(Cross(Vector3Subtract(b, a), Vector3Subtract(c, a)));
        //Plane face = brush->planes[triangleFaceIndex[i / 3]];
        for (int j = 0; j < 3; j++) {
            Vector3 v = verts[i + j];
            int idx = i + j;

            mesh.vertices[idx * 3 + 0] = v.x;
            mesh.vertices[idx * 3 + 1] = v.y;
            mesh.vertices[idx * 3 + 2] = v.z;

            mesh.normals[idx * 3 + 0] = normal.x;
            mesh.normals[idx * 3 + 1] = normal.y;
            mesh.normals[idx * 3 + 2] = normal.z;

            // Get face info
            Plane face = brush->planes[vertexFaceIndex[idx]];

            // Find a consistent origin for this face
            // Ideally: intersection of face.plane and two other planes — but we’ll fake it using verts[i]
            Vector3 faceOrigin = verts[i];  // local origin for projection

            // Compute vector from face origin to current vertex
            Vector3 rel = Vector3Subtract(v, faceOrigin);

            // Project onto texture axes, apply scale and offset
            float u = Vector3DotProduct(rel, face.texU) / face.scaleU + face.offsetU;
            float v_coord = Vector3DotProduct(rel, face.texV) / face.scaleV + face.offsetV;

            // Flip V to match Raylib’s coordinate system
            v_coord = 1.0f - v_coord;
            
            mesh.texcoords[idx * 2 + 0] = u;
            mesh.texcoords[idx * 2 + 1] = v_coord;
            //printf("texcoords - u=%f v=%f\n", u, v_coord);
        }
    }


    //if web, scale the values of texcoords
    #ifdef PLATFORM_WEB
        float minU = 1000000.0f, maxU = -1000000.0f;
        float minV = 1000000.0f, maxV = -1000000.0f;

        for (int i = 0; i < mesh.vertexCount; i++) {
            float u = mesh.texcoords[i * 2 + 0];
            float v = mesh.texcoords[i * 2 + 1];

            if (u < minU) minU = u;
            if (u > maxU) maxU = u;
            if (v < minV) minV = v;
            if (v > maxV) maxV = v;
        }

        float scaleU = (maxU - minU != 0.0f) ? (1.0f / (maxU - minU)) : 1.0f;
        float scaleV = (maxV - minV != 0.0f) ? (1.0f / (maxV - minV)) : 1.0f;

        for (int i = 0; i < mesh.vertexCount; i++) {
            mesh.texcoords[i * 2 + 0] = (mesh.texcoords[i * 2 + 0] - minU) * scaleU * 5;
            mesh.texcoords[i * 2 + 1] = (mesh.texcoords[i * 2 + 1] - minV) * scaleV * 5;
        }
    #endif 

    UploadMesh(&mesh, false);
    return mesh;
}

// -----------------------------
// .MAP Parser
// -----------------------------

Entity* LoadMapFile(const char *filename, int *modelCount) {
   
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        TraceLog(LOG_ERROR, "Could not open %s", filename);
        *modelCount = 0;
        return NULL;
    }

    Brush *brushes = MemAlloc(sizeof(Brush) * MAX_BRUSHES);
    int brushCount = 0;
    bool inBrush = false;
    Brush current = {0};
    bool hasClassName = false;
    char currentClassName[64];
    bool hasOrigin = false;
    Vector3 currentOrigin;
    bool hasSubType = false;
    char currentSubType[64];
    char line[512];
    int entityDepth = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strchr(line, '{')) {
            entityDepth++;
            //TraceLog(LOG_INFO, "Open brace (depth %d)", entityDepth);
            if (entityDepth == 2) {
                inBrush = true;
                current.planeCount = 0;
            }
        } else if (strchr(line, '}')) {
            //TraceLog(LOG_INFO, "Close brace (depth %d)", entityDepth);
            if (inBrush && entityDepth == 2 && current.planeCount > 0)
            {
                TraceLog(LOG_INFO, "Finalized brush with %d planes", current.planeCount);
                strcpy(current.className,currentClassName);
                current.hasOrigin = hasOrigin;
                if(hasOrigin){current.origin = currentOrigin;}
                brushes[brushCount++] = current;
                inBrush = false;
            }
            else if(entityDepth == 1 && hasOrigin && hasClassName) //point entity
            {
                current.hasOrigin = hasOrigin;
                current.hasSubType = hasSubType;//subtypes only on point entities
                if(hasSubType){strcpy(current.subType,currentSubType);}
                strcpy(current.className,currentClassName);
                if(hasOrigin){current.origin = currentOrigin;}
                current.planeCount = 0;
                brushes[brushCount++] = current;
            }
            hasClassName = false;//these are only needed ro point entities so safe to switch off here
            hasOrigin = false;//these are only needed ro point entities so safe to switch off here
            entityDepth--;
        } else if (inBrush && entityDepth == 2) {
            Vector3 a, b, c;
            char texName[64];
            float u[4], v[4];
            float rotation, scaleX, scaleY;

            // Skip to start of face
            char *s = strchr(line, '(');
            if (s) {
                int matched = sscanf(s,
                    "( %f %f %f ) ( %f %f %f ) ( %f %f %f ) %63s [ %f %f %f %f ] [ %f %f %f %f ] %f %f %f",
                    &a.x, &a.y, &a.z,
                    &b.x, &b.y, &b.z,
                    &c.x, &c.y, &c.z,
                    texName,
                    &u[0], &u[1], &u[2], &u[3],
                    &v[0], &v[1], &v[2], &v[3],
                    &rotation, &scaleX, &scaleY);
                //TraceLog(LOG_INFO, "matched: %d", matched);
                if (matched == 21) {
                    Plane pl = PlaneFromPoints(a, b, c);
                    strncpy(pl.textureName, texName, 63);
                    pl.textureName[63] = '\0';

                    // Valve 220 axes
                    // Convert axes into Raylib coordinate space
                    // Convert axes to Raylib (Y-up) space but DO NOT scale
                    Vector3 rawU = ConvertFromQuake((Vector3){ u[0], u[1], u[2] });
                    Vector3 rawV = ConvertFromQuake((Vector3){ v[0], v[1], v[2] });

                    // Rotate axes on the plane
                    Vector3 normal = pl.normal;
                    //todo: these lines were the cause of a big headache
                    // ... but, are they needed and if so what is actually wrong with them ...
                    //rawU = RotateVectorAroundAxis(rawU, normal, rotation); 
                    //rawV = RotateVectorAroundAxis(rawV, normal, rotation);

                    // Don't scale here — scale is for pixel density, not world units
                    pl.texU = Vector3Scale(rawU, 1.0f / scaleX);
                    pl.texV = Vector3Scale(rawV, 1.0f / scaleY);

                    // Do NOT convert offsets to meters — they stay in texture space
                    pl.offsetU = u[3] / scaleX;
                    pl.offsetV = v[3] / scaleY;

                    pl.scaleU = scaleX;
                    pl.scaleV = scaleY;
                    //debug code, todo, remove
                    // pl.texU = (Vector3){ 1, 0, 0 };
                    // pl.texV = (Vector3){ 0, 1, 0 };
                    // pl.offsetU = 0;
                    // pl.offsetV = 0;

                    if (current.planeCount < MAX_PLANES) {
                        current.planes[current.planeCount++] = pl;
                    }
                }
            } else {
                TraceLog(LOG_INFO, "Ignored line: %s", line);
            }
        } 
        else if(entityDepth == 1)
        {
            char key[64];
            char value[64];

            int matched = sscanf(line, "\"%63[^\"]\" \"%63[^\"]\"", key, value);

            if (matched == 2 && strcmp(key, "classname") == 0)
            {
                // Make sure currentClassName is big enough and allocated
                strncpy(currentClassName, value, 63);
                currentClassName[63] = '\0'; // Null-terminate just in case
                printf("found classname: %s\n", currentClassName);
                hasClassName = true;
            }
            else if (matched == 2 && strcmp(key, "origin") == 0)
            {
                printf("found origin: %s\n", value);
                Vector3 vec;
                int m = sscanf(value, "%f %f %f", &vec.x,&vec.y,&vec.z);
                if(m == 3)
                {
                    printf("has origin marked\n");
                    hasOrigin = true;
                    currentOrigin = Vector3Scale(ConvertFromQuake(vec), QUAKE_TO_METERS);
                    //PrintVector3("position: ", currentOrigin);
                }
            }
            else if (matched == 2 && strcmp(key, "subtype") == 0)
            {
                strncpy(currentSubType, value, 63);
                currentSubType[63] = '\0'; // Null-terminate just in case
                printf("found subtype: %s\n", currentSubType);
                hasSubType = true;
            }
        }
    }



    fclose(fp);

    *modelCount = brushCount;
    Entity *entities = malloc(sizeof(Entity) * brushCount);
    for (int i = 0; i < brushCount; i++) {
        if(brushes[i].planeCount > 0)
        {
            Mesh m = BuildMeshFromBrush(&brushes[i]);
            TraceLog(LOG_INFO, "Model %d: %d triangles", i, m.triangleCount);
            Model mod = LoadModelFromMesh(m);
            entities[i].model = mod;
        }
        strcpy(entities[i].className,brushes[i].className);
        entities[i].hasOrigin = brushes[i].hasOrigin;
        entities[i].origin = brushes[i].origin;
        entities[i].hasSubType = brushes[i].hasSubType;
        if(brushes[i].hasSubType){strcpy(entities[i].subType,brushes[i].subType);}
    }

    MemFree(brushes); //get rid of those pesky brushes

    return entities;
}
