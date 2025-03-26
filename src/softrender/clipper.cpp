#include"render.h"

namespace ClipTools{
// Defines a bit flag for the clipping plane
enum ClipPlaneBit {
    CLIP_LEFT   = 1 << 0, // 000001
    CLIP_RIGHT  = 1 << 1, // 000010
    CLIP_BOTTOM = 1 << 2, // 000100
    CLIP_TOP    = 1 << 3, // 001000
    CLIP_NEAR   = 1 << 4, // 010000
    CLIP_FAR    = 1 << 5  // 100000
};

int computeOutcode(const glm::vec4& pos) {
    int outcode = 0;
    if (pos.x < -pos.w) outcode |= CLIP_LEFT;
    if (pos.x > pos.w)  outcode |= CLIP_RIGHT;
    if (pos.y < -pos.w) outcode |= CLIP_BOTTOM;
    if (pos.y > pos.w)  outcode |= CLIP_TOP;
    if (pos.z < -pos.w) outcode |= CLIP_NEAR;
    if (pos.z > pos.w)  outcode |= CLIP_FAR;
    return outcode;
}

bool isInside(const Vertex& vertex, ClipPlane plane) {
    const glm::vec4& pos = vertex.c_pos_;
    switch (plane) {
        case ClipPlane::Left:
            return pos.x >= -pos.w;
        case ClipPlane::Right:
            return pos.x <= pos.w;
        case ClipPlane::Bottom:
            return pos.y >= -pos.w;
        case ClipPlane::Top:
            return pos.y <= pos.w;
        case ClipPlane::Near:
            return pos.z >= -pos.w;
        case ClipPlane::Far:
            return pos.z <= pos.w;
        default:
            return false;
    }
}

bool computeIntersection(const Vertex& v1, const Vertex& v2,Vertex& v3, ClipPlane plane) {
    float A, B, C, D;
    switch (plane) {
        case ClipPlane::Left:
            A = 1.0f; B = 0.0f; C = 0.0f; D = 1.0f;
            break;
        case ClipPlane::Right:
            A = -1.0f; B = 0.0f; C = 0.0f; D = 1.0f;
            break;
        case ClipPlane::Bottom:
            A = 0.0f; B = 1.0f; C = 0.0f; D = 1.0f;
            break;
        case ClipPlane::Top:
            A = 0.0f; B = -1.0f; C = 0.0f; D = 1.0f;
            break;
        case ClipPlane::Near:
            A = 0.0f; B = 0.0f; C = 1.0f; D = 1.0f;
            break;
        case ClipPlane::Far:
            A = 0.0f; B = 0.0f; C = -1.0f; D = 1.0f;
            break;
        default:
            A = B = C = D = 0.0f;
    }

    float startVal = A * v1.c_pos_.x + B * v1.c_pos_.y + C * v1.c_pos_.z + D * v1.c_pos_.w;
    float endVal = A * v2.c_pos_.x + B * v2.c_pos_.y + C * v2.c_pos_.z + D * v2.c_pos_.w;

    // denom check
    if (fabs(startVal - endVal) < 1e-6f) {
        return false;
    }

    float t = startVal / (startVal - endVal);
    t = glm::clamp(t, 0.0f, 1.0f);
    
    v1.vertexInterp(v2, v3,t);
    return true;
}
}


void Render::clipWithPlane(ClipPlane plane, std::vector<Vertex>& in, std::vector<Vertex>& out) {
    if (in.empty()) return;

    std::vector<Vertex> result;
    size_t vnum = in.size();

    for (size_t i = 0; i < vnum; ++i) {
        size_t next = (i + 1) % vnum;
        const Vertex& current = in[i];
        const Vertex& nextPos = in[next];

        bool currentInside = ClipTools::isInside(current, plane);
        bool nextInside = ClipTools::isInside(nextPos, plane);

        if (currentInside && nextInside) {
            // Case 1: Both inside
            result.push_back(nextPos);
        }
        else if (currentInside && !nextInside) {
            // Case 2: Current inside, next outside
            Vertex intersectVertex;
            bool flag=ClipTools::computeIntersection(current, nextPos,intersectVertex,plane);
            if(flag) result.push_back(intersectVertex);
        }
        else if (!currentInside && nextInside) {
            // Case 3: Current outside, next inside
            Vertex intersectVertex;
            bool flag=ClipTools::computeIntersection(current, nextPos, intersectVertex,plane);
            if(flag) result.push_back(intersectVertex);
            result.push_back(nextPos);
        }
        // Case 4: Both outside - do nothing
    }

    out = std::move(result);
}

// return the number of triangles after clipping
int Render::pipelineClipping(std::vector<Vertex>& vertices, std::vector<Vertex>& out) {
    assert(vertices.size()==3);

    // get outcode
    int outcode1 = ClipTools::computeOutcode(vertices[0].c_pos_);
    int outcode2 = ClipTools::computeOutcode(vertices[1].c_pos_);
    int outcode3 = ClipTools::computeOutcode(vertices[2].c_pos_);

    int outcode_OR = outcode1 | outcode2 | outcode3;
    int outcode_AND = outcode1 & outcode2 & outcode3;

    // rapid reject
    if (outcode_AND != 0) {
        return 0; // the triangle is totally outside
    }

    // rapid accept
    if (outcode_OR == 0) {
        out = vertices;
        return 1;
    }

    // clip
    std::vector<Vertex> input = vertices;
    std::vector<Vertex> temp;

    // define order
    std::vector<ClipPlane> planes = {
        ClipPlane::Left,
        ClipPlane::Right,
        ClipPlane::Bottom,
        ClipPlane::Top,
        ClipPlane::Near,
        ClipPlane::Far
    };

    for (const auto& plane : planes) {
        temp.clear();
        clipWithPlane(plane, input, temp);
        input = std::move(temp);

        if (input.empty()) {
            out.clear();
            return 0;
        }
    }


    int vnum = input.size();
    out.clear();
    if (vnum < 3) return 0;

    for (int i = 1; i < vnum - 1; ++i) {
        out.push_back(input[0]);
        out.push_back(input[i]);
        out.push_back(input[i + 1]);
    }

    return out.size() / 3;
}

// in screen space
bool Render::backCulling(const glm::vec3& face_norm,const glm::vec3& dir) const {
    return glm::dot(dir,face_norm) <= 0;
}

// backculling and frustrum culling
void Render::cullingTriangleInstance(ASInstance& instance,const glm::mat4 normal_mat){
    instance.refreshVertices();
    info_.profile_.total_face_num_+=instance.blas_->object_->getFaceNum();

    auto& obj=instance.blas_->object_;
    const std::vector<Vertex>& in_vertices=obj->getconstVertices();
    const std::vector<uint32_t>& in_indices=obj->getIndices();
    std::vector<glm::vec3>& objfacenorms=obj->getFaceNorms();
    const std::vector<int>& in_mtlidx=obj->getMtlIdx();

    std::vector<Vertex>& out_vertices=*instance.vertices_;
     std::vector<PrimitiveHolder>& out_primitives_buffer=*instance.primitives_buffer_;

    uint32_t ver_num=in_vertices.size();
    uint32_t idx_num=in_indices.size();
    uint32_t face_num=in_mtlidx.size();
    uint32_t primitive_num=instance.blas_->primitives_indices_->size();

    // reserve some space
    out_vertices.reserve(ver_num+100);
    out_primitives_buffer.reserve(primitive_num+10);

    assert(idx_num%3==0&&idx_num/3==primitive_num);

    // define clipping order
    std::vector<ClipPlane> planes = {
        ClipPlane::Left,
        ClipPlane::Right,
        ClipPlane::Bottom,
        ClipPlane::Top,
        ClipPlane::Near,
        ClipPlane::Far
    };


    for(int indices_offset=0;indices_offset<idx_num;indices_offset+=3){
    // clipping each triangle.
        int face_cnt=indices_offset/3;

        uint32_t idx1=in_indices[indices_offset+0];
        uint32_t idx2=in_indices[indices_offset+1];
        uint32_t idx3=in_indices[indices_offset+2];

        auto& v1=in_vertices[idx1];
        auto& v2=in_vertices[idx2];
        auto& v3=in_vertices[idx3];

        /* ---------------- back culling ---------------- */
        if(info_.raster_setting_.back_culling&&obj->isBackCulling()){

            glm::vec3 norm=normal_mat*glm::vec4(objfacenorms[face_cnt],0.f);
            glm::vec3 dir=camera_.getPosition()-v1.w_pos_;
            if(backCulling(norm,dir)==true){
                ++info_.profile_.back_culled_face_num_;
                out_primitives_buffer.emplace_back(ClipFlag::refused,
                                                -1,// mtlidx_
                                                0, // vertex_start_pos_
                                                0);// vertex_num_
                continue;
            }
        }

        /* -------------- frustrum culling -------------- */
        // get outcode
        int outcode1 = ClipTools::computeOutcode(v1.c_pos_);
        int outcode2 = ClipTools::computeOutcode(v2.c_pos_);
        int outcode3 = ClipTools::computeOutcode(v3.c_pos_);

        int outcode_OR = outcode1 | outcode2 | outcode3;
        int outcode_AND = outcode1 & outcode2 & outcode3;

        // rapid reject
        if (outcode_AND != 0) {
            ++info_.profile_.clipped_face_num_;
            out_primitives_buffer.emplace_back(ClipFlag::refused,
                                                -1,// mtlidx_
                                                0, // vertex_start_pos_
                                                0);// vertex_num_
            continue;
        }

        // rapid accept
        if (outcode_OR == 0) {
            out_primitives_buffer.emplace_back(ClipFlag::accecpted,
                                                in_mtlidx[face_cnt], // mtlidx_
                                                out_vertices.size(), // vertex_start_pos_
                                                3);                  // vertex_num_
            out_vertices.emplace_back(v1);
            out_vertices.emplace_back(v2);
            out_vertices.emplace_back(v3);

            continue;
        }

        std::vector<Vertex> input{v1,v2,v3};
        std::vector<Vertex> temp;

        bool clipflag=false;
        for (const auto& plane : planes) {// TODO: use mask to reduce unneccessary clipping
            temp.clear();
            clipWithPlane(plane, input, temp);
            input = std::move(temp);

            if (input.empty()) {
                clipflag=true;
                break;
            }
        }
        // totally clipped out
        if(clipflag){
            ++info_.profile_.clipped_face_num_;
            out_primitives_buffer.emplace_back(ClipFlag::refused,
                                            -1,// mtlidx_
                                            0, // vertex_start_pos_
                                            0);// vertex_num_
            continue;
        }

        int vnum = input.size();
        if (vnum < 3){
             ++info_.profile_.clipped_face_num_;
            out_primitives_buffer.emplace_back(ClipFlag::refused,
                                                -1,// mtlidx_
                                                0, // vertex_start_pos_
                                                0);// vertex_num_
            continue;
        }

        // been clipped into pieaces
        int vertex_start_pos=out_vertices.size();
        for (int i = 1; i < vnum - 1; ++i) {
            out_vertices.emplace_back(input[0]);
            out_vertices.emplace_back(input[i]);
            out_vertices.emplace_back(input[i + 1]);
        }
        info_.profile_.total_face_num_+=vnum-3;
        out_primitives_buffer.emplace_back(ClipFlag::clipped,
                                            in_mtlidx[face_cnt], // mtlidx_
                                            vertex_start_pos,    // vertex_start_pos_
                                            3*(vnum-2));         // vertex_num_
    }

    assert(out_vertices.size()%3==0);
    assert(out_primitives_buffer.size()==primitive_num);

}


