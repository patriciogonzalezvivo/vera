
#include "vera/io/stl.h"

#include <iostream>
#include <fstream>
#include <string>

#include "vera/ops/geom.h"
#include "vera/ops/string.h"

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

namespace vera {

bool loadSTL(const std::string& _filename, Scene* _scene, bool _verbose) {
    std::string name = _filename.substr(0, _filename.size()-4);

    if (_scene->materials.find("default") == _scene->materials.end())
        _scene->materials["default"] = new Material("default");
        
    Mesh mesh;
    Material* default_material = _scene->materials["default"];

    FILE * stl_file = fopen(_filename.c_str(),"rb");
    if (NULL == stl_file) {
        fprintf(stderr,"IOError: %s could not be opened...\n", _filename.c_str());
        return false;
    }

     // Specifically 80 character header
    char header[80];
    char solid[80] = {0};
    bool is_ascii = true;
    if (fread(header, 1, 80, stl_file) != 80) {
        std::cerr << "IOError: too short (1)." << std::endl;
        goto close_false;
    }
    sscanf(header, "%79s", solid);
  
    if ( std::string("solid") != solid) 
        is_ascii = false;
    else {
        // might still be binary
        char buf[4];
        if ( fread(buf, 1, 4, stl_file) != 4) {
            std::cerr << "IOError: too short (3)." << std::endl;
            goto close_false;
        }
    
        size_t num_faces = *reinterpret_cast<unsigned int*>(buf);
        fseek(stl_file,0,SEEK_END);
        size_t file_size = ftell(stl_file);
        if (file_size == 80 + 4 + (4*12 + 2) * num_faces) is_ascii = false;
        else is_ascii = true;
    }

    if (is_ascii) {
        // Rewind to end of header
        //stl_file = fopen(filename.c_str(),"r");
        //stl_file = freopen(NULL,"r",stl_file);
        fseek(stl_file, 0, SEEK_SET);
        if (NULL == stl_file) {
            fprintf(stderr,"IOError: stl file could not be reopened as ascii ...\n");
            return false;
        }
        // Read 80 header
        // Eat file name

        char name[LINE_MAX];
        if ( NULL == fgets(name, LINE_MAX, stl_file)) {
            std::cerr << "IOError: ascii too short (2)." << std::endl;
            goto close_false;
        }

        // ascii
        while(true) {
            int ret;
            char facet[LINE_MAX];
            char normal[LINE_MAX];

            glm::vec3 n;

            double nd[3];
            ret = fscanf(stl_file,"%s %s %lg %lg %lg", facet, normal, nd, nd+1, nd+2);
            n.x = nd[0];
            n.y = nd[2];
            n.z = -nd[1];

            if (std::string("endsolid") == facet)
                break;
            
            if (ret != 5 || 
                !(  std::string("facet") == facet || 
                    std::string("faced") == facet) ||
                    std::string("normal") != normal) {
                std::cout << "facet: " << facet << std::endl;
                std::cout << "normal: " <<normal << std::endl;
                std::cerr << "IOError: bad format (1)." << std::endl;
                goto close_false;
            }
            // copy casts to Type
            n.x = nd[0]; 
            n.y = nd[2];
            n.z = -nd[1];

            mesh.addNormal(n);
            mesh.addNormal(n);
            mesh.addNormal(n);
            char outer[LINE_MAX], loop[LINE_MAX];
            ret = fscanf(stl_file,"%s %s",outer,loop);
            if (ret != 2 || std::string("outer") != outer || std::string("loop") != loop) {
                std::cerr << "IOError: bad format (2). " << std::endl;
                goto close_false;
            }
            
            while(true) {
                char word[LINE_MAX];
                int ret = fscanf(stl_file, "%s", word);
                if (ret == 1 && std::string("endloop") == word)
                    break;
                else if (ret == 1 && std::string("vertex") == word) {
                    double vd[3];
                    int ret = fscanf(stl_file, "%lg %lg %lg", vd, vd+1, vd+2);
                    if (ret != 3) {
                        std::cerr << "IOError: bad format (3)." << std::endl;
                        goto close_false;
                    }
                    mesh.addVertex(glm::vec3(vd[0], vd[2], -vd[1]));
                }
                else {
                    std::cerr << "IOError: bad format (4)." << std::endl;
                    goto close_false;
                }
            }

            char endfacet[LINE_MAX];
            ret = fscanf(stl_file,"%s",endfacet);
            if (ret != 1 || std::string("endfacet") != endfacet) {
                std::cerr << "IOError: bad format (5)." << std::endl;
                goto close_false;
            }
        }
        // read endfacet
        goto close_true;
    }
    else {
        // Binary
        // stl_file = freopen(NULL,"rb",stl_file);
        fseek(stl_file, 0, SEEK_SET);
        // Read 80 header
        char header[80];
        if (fread(header, sizeof(char), 80, stl_file) != 80) {
            std::cerr << "IOError: bad format (6)." << std::endl;
            goto close_false;
        }
        // Read number of triangles
        unsigned int num_tri;
        if (fread(&num_tri, sizeof(unsigned int), 1, stl_file) != 1) {
            std::cerr << "IOError: bad format (7)." << std::endl;
            goto close_false;
        }

        for(int t = 0; t < (int)num_tri; t++) {
            // Read normal
            glm::vec3 n;
            if (fread(&n.x, sizeof(float), 3, stl_file) != 3) {
                std::cerr << "IOError: bad format (8)." << std::endl;
                goto close_false;
            }
            mesh.addNormal(glm::vec3(n.x, n.z, -n.y));
            mesh.addNormal(glm::vec3(n.x, n.z, -n.y));
            mesh.addNormal(glm::vec3(n.x, n.z, -n.y));

            // Read each vertex
            for (size_t c = 0; c < 3; c++) {
                glm::vec3 v;
                if (fread (&v.x, sizeof(float), 3, stl_file) != 3) {
                    std::cerr << "IOError: bad format (9)." << std::endl;
                    goto close_false;
                }
                mesh.addVertex(glm::vec3(v.x, v.z, -v.y));
            }
            
            // Read attribute size
            unsigned short att_count;
            if (fread(&att_count, sizeof(unsigned short), 1, stl_file) != 1) {
                std::cerr << "IOError: bad format (10)." << std::endl;
                goto close_false;
            }
        }
        goto close_true;
    }

    close_false:
        fclose(stl_file);
        return false;
    close_true:
        fclose(stl_file);
        _scene->models[name] = new Model(name, mesh, default_material);
        return true;
}


bool loadSTL( const std::string& _filename, Mesh& _mesh ) {
    FILE * stl_file = fopen(_filename.c_str(),"rb");
    if (NULL == stl_file) {
        fprintf(stderr,"IOError: %s could not be opened...\n", _filename.c_str());
        return false;
    }

     // Specifically 80 character header
    char header[80];
    char solid[80] = {0};
    bool is_ascii = true;
    if (fread(header, 1, 80, stl_file) != 80) {
        std::cerr << "IOError: too short (1)." << std::endl;
        goto close_false;
    }
    sscanf(header, "%79s", solid);
  
    if ( std::string("solid") != solid) 
        is_ascii = false;
    else {
        // might still be binary
        char buf[4];
        if ( fread(buf, 1, 4, stl_file) != 4) {
            std::cerr << "IOError: too short (3)." << std::endl;
            goto close_false;
        }
    
        size_t num_faces = *reinterpret_cast<unsigned int*>(buf);
        fseek(stl_file,0,SEEK_END);
        int file_size = ftell(stl_file);
        if (file_size == 80 + 4 + (4*12 + 2) * num_faces) is_ascii = false;
        else is_ascii = true;
    }

    if (is_ascii) {
        // Rewind to end of header
        //stl_file = fopen(filename.c_str(),"r");
        //stl_file = freopen(NULL,"r",stl_file);
        fseek(stl_file, 0, SEEK_SET);
        if (NULL == stl_file) {
            fprintf(stderr,"IOError: stl file could not be reopened as ascii ...\n");
            return false;
        }
        // Read 80 header
        // Eat file name

        char name[LINE_MAX];
        if ( NULL == fgets(name, LINE_MAX, stl_file)) {
            std::cerr << "IOError: ascii too short (2)." << std::endl;
            goto close_false;
        }

        // ascii
        while(true) {
            int ret;
            char facet[LINE_MAX];
            char normal[LINE_MAX];

            Triangle tri;
            glm::vec3 n;

            double nd[3];
            ret = fscanf(stl_file,"%s %s %lg %lg %lg", facet, normal, nd, nd+1, nd+2);
            n.x = nd[0];
            n.y = nd[1];
            n.z = nd[2];

            if (std::string("endsolid") == facet)
                break;
            
            if (ret != 5 || 
                !(  std::string("facet") == facet || 
                    std::string("faced") == facet) ||
                    std::string("normal") != normal) {
                std::cout << "facet: " << facet << std::endl;
                std::cout << "normal: " <<normal << std::endl;
                std::cerr << "IOError: bad format (1)." << std::endl;
                goto close_false;
            }
            // copy casts to Type
            n.x = nd[0]; 
            n.y = nd[1];
            n.z = nd[2];
            tri.setNormals(n, n, n);
            char outer[LINE_MAX], loop[LINE_MAX];
            ret = fscanf(stl_file,"%s %s",outer,loop);
            if (ret != 2 || std::string("outer") != outer || std::string("loop") != loop) {
                std::cerr << "IOError: bad format (2). " << std::endl;
                goto close_false;
            }
            
            size_t index = 0;
            while(true) {
                char word[LINE_MAX];
                int ret = fscanf(stl_file, "%s", word);
                if (ret == 1 && std::string("endloop") == word)
                    break;
                else if (ret == 1 && std::string("vertex") == word) {
                    glm::vec3 v;
                    double vd[3];
                    int ret = fscanf(stl_file, "%lg %lg %lg", vd, vd+1, vd+2);
                    if (ret != 3) {
                        std::cerr << "IOError: bad format (3)." << std::endl;
                        goto close_false;
                    }
                    tri.setVertex(index, glm::vec3(vd[0], vd[1], vd[2]));
                    index++;
                }
                else {
                    std::cerr << "IOError: bad format (4)." << std::endl;
                    goto close_false;
                }
            }
            _mesh.addTriangle(tri);

            char endfacet[LINE_MAX];
            ret = fscanf(stl_file,"%s",endfacet);
            if (ret != 1 || std::string("endfacet") != endfacet) {
                std::cerr << "IOError: bad format (5)." << std::endl;
                goto close_false;
            }
        }
        // read endfacet
        goto close_true;
    }
    else {
        // Binary
        // stl_file = freopen(NULL,"rb",stl_file);
        fseek(stl_file, 0, SEEK_SET);
        // Read 80 header
        char header[80];
        if (fread(header, sizeof(char), 80, stl_file) != 80) {
            std::cerr << "IOError: bad format (6)." << std::endl;
            goto close_false;
        }
        // Read number of triangles
        unsigned int num_tri;
        if (fread(&num_tri, sizeof(unsigned int), 1, stl_file) != 1) {
            std::cerr << "IOError: bad format (7)." << std::endl;
            goto close_false;
        }

        std::vector<Triangle> triangles;
        triangles.resize(num_tri);
        for(int t = 0; t < (int)num_tri; t++) {
            Triangle tri;

            // Read normal
            glm::vec3 n;
            if (fread(&n.x, sizeof(float), 3, stl_file) != 3) {
                std::cerr << "IOError: bad format (8)." << std::endl;
                goto close_false;
            }
            tri.setNormals(n, n, n);

            // Read each vertex
            for (size_t c = 0; c < 3; c++) {
                glm::vec3 v;
                if (fread (&v.x, sizeof(float), 3, stl_file) != 3) {
                    std::cerr << "IOError: bad format (9)." << std::endl;
                    goto close_false;
                }
                tri.setVertex(c, v);
            }
            
            // Read attribute size
            unsigned short att_count;
            if (fread(&att_count, sizeof(unsigned short), 1, stl_file) != 1) {
                std::cerr << "IOError: bad format (10)." << std::endl;
                goto close_false;
            }
        }
        goto close_true;
    }

    close_false:
        fclose(stl_file);
        return false;
    close_true:
        fclose(stl_file);
        return true;
}

// bool saveStl( const std::string& _filename, const Mesh& _mesh, bool _binnary ) {
//     std::vector<Triangle> triangles = _mesh.getTriangles();

//     if (!_binnary) {
//         FILE * stl_file = fopen(_filename.c_str(),"w");
//         if (stl_file == NULL) {
//             std::cerr <<"IOError: " << _filename << " could not be opened for writing."<< std::endl;
//             return false;
//         }

//         fprintf(stl_file,"solid %s\n", _filename.c_str());
//         for (size_t i = 0; i < triangles.size(); i++) {
//             fprintf(stl_file,"facet normal %e %e %e\n",
//                     (float)triangles[i].getNormal().x,
//                     (float)triangles[i].getNormal().y,
//                     (float)triangles[i].getNormal().z);

//             fprintf(stl_file,"outer loop\n");
//             for (int v = 0; v < 3; v++) {
//                 fprintf(stl_file,"vertex %e %e %e\n",
//                                 (float)triangles[i][v].x,
//                                 (float)triangles[i][v].y,
//                                 (float)triangles[i][v].z);
//             }
//             fprintf(stl_file,"endloop\n");
//             fprintf(stl_file,"endfacet\n");
//         }
//         fprintf(stl_file,"endsolid %s\n", _filename.c_str());
//         fclose(stl_file);
//         return true;
//     }
//     else {
//         FILE * stl_file = fopen(_filename.c_str(),"wb");
//         if (stl_file == NULL) {
//             std::cerr << "IOError: " << _filename << " could not be opened for writing." << std::endl;
//             return false;
//         }

//         // Write unused 80-char header
//         for (char h = 0;h<80;h++)
//             fwrite(&h,sizeof(char), 1, stl_file);

//         // Write number of triangles
//         unsigned int num_tri = triangles.size();
//         fwrite(&num_tri, sizeof(unsigned int), 1, stl_file);
//         // assert(F.cols() == 3);

//         // Write each triangle
//         for (size_t i = 0; i < triangles.size(); i++) {
//             fwrite(&triangles[i].getNormal().x, sizeof(float), 3, stl_file);
//             for (int v = 0; v < 3; v++)
//                 fwrite(&triangles[i][v].x, sizeof(float), 3, stl_file);

//             unsigned short att_count = 0;
//             fwrite(&att_count, sizeof(unsigned short), 1, stl_file);
//         }
//         fclose(stl_file);
//         return true;
//     }
// }


}