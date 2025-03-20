#include "render.h"

Render::Render() : camera_(), colorbuffer_(std::make_shared<ColorBuffer>(camera_.getImageWidth(), camera_.getImageHeight())),
                   zbuffer_(std::make_shared<DepthBuffer>(camera_.getImageWidth(), camera_.getImageHeight())),
                   info_()
{

    box2d_.min = {0, 0};
    box2d_.max = {camera_.getImageWidth() - 1, camera_.getImageHeight() - 1};
    box3d_.min = {0, 0, -1};
    box3d_.max = {camera_.getImageWidth() - 1, camera_.getImageHeight() - 1, 1};
}

// once change camera property, we need to update VPV-matrix accordingly
void Render::updateMatrix()
{
    mat_view_ = camera_.getViewMatrix();
    mat_perspective_ = camera_.getPerspectiveMatrix();
    mat_viewport_ = camera_.getViewportMatrix();
}

void Render::addObjInstance(std::string filename, glm::mat4 &model, ShaderType shader, bool flipn, bool backculling)
{
    // scene_.addObjInstance(filename,model,shader,flipn,backculling);
    scene_.addObjInstance_SpaceFriendly(filename, model, shader, flipn, backculling);
}

void Render::pipelineInit()
{
    // get default setting_
    initRenderIoInfo();

    // 0. load scene and camera setting
    loadDemoScene(info_.raster_setting_.scene_filename, info_.raster_setting_.shader_type);
    setBVHLeafSize(info_.raster_setting_.bvh_leaf_num);
    scene_.buildTLAS();

    // // 1. init transformation
    // updateMatrix();

    // 2. init shader
    sdptr_ = std::make_shared<Shader>();
    sdptr_->bindTimer(&info_.rasterize_timer_);

    // 3. init rastertizer
    tri_scanliner_ = std::make_shared<ScanLine::PerTriangleScanLiner>(box2d_, colorbuffer_, zbuffer_, sdptr_);

    if (info_.raster_setting_.rasterize_type == RasterizeType::Easy_hzb ||
        info_.raster_setting_.rasterize_type == RasterizeType::Bvh_hzb)
    {
        hzb_ = std::make_shared<HZbuffer>(camera_.getImageWidth(), camera_.getImageHeight());
    }

    is_init_ = true;
}

// drawLine in screen space
void Render::drawLine(glm::vec2 t1, glm::vec2 t2)
{
    glm::vec4 color(255.0f);
    // make sure: x-axis is less steep and t1 is the left point
    bool swap_flag = 0;
    if (std::abs(t1.x - t2.x) < std::abs(t1.y - t2.y))
    {
        std::swap(t1.x, t1.y);
        std::swap(t2.x, t2.y);
        swap_flag = 1;
    }
    if (t1.x > t2.x)
    {
        std::swap(t1, t2);
    }
    bool positive_flag = 1;
    int dx = t2.x - t1.x;
    int dy = t2.y - t1.y;
    if (dy < 0)
    {
        dy = -dy;
        positive_flag = 0;
    }

    int delta2 = dy * 2;
    int error2 = 0;
    int y = t1.y;
    for (int x = t1.x; x <= t2.x; ++x)
    {
        if (swap_flag)
        {
            colorbuffer_->setPixel(y, x, color);
        }
        else
        {
            colorbuffer_->setPixel(x, y, color);
        }
        error2 += delta2;
        if (error2 > dx)
        {
            y += positive_flag ? 1 : -1;
            error2 -= dx * 2;
        }
    }
}

// go through all the pixels inside the AABB, that means I didn't use coherence here
void Render::drawTriangleNaive()
{
    ++info_.profile_.shaded_face_num_;

    glm::vec3 t[3];
    for (int i = 0; i < 3; ++i)
        t[i] = sdptr_->getScreenPos(i);

    AABB3d aabb(t[0], t[1], t[2]);
    aabb.clipAABB(box3d_);

    if (aabb.min.x >= aabb.max.x || aabb.min.y >= aabb.max.y)
        return;

    if (ShaderType::Frame == sdptr_->getType())
    {
        for (int i = 0; i < 3; ++i)
        {
            drawLine(t[i], t[(i + 1) % 3]);
        }
    }
    else
    {
        for (int y = aabb.min.y; y <= aabb.max.y; ++y)
        {
            for (int x = aabb.min.x; x <= aabb.max.x; ++x)
            {

                float depth = sdptr_->fragmentDepth(x, y);
                if (zbuffer_->zTest(x, y, depth))
                {
                    sdptr_->fragmentShader(x, y);
                    colorbuffer_->setPixel(x, y, sdptr_->getColor());
                }
            }
        }
    }
}

void Render::drawTriangleHZB()
{
    ++info_.profile_.shaded_face_num_;
    glm::vec3 t[3];
    for (int i = 0; i < 3; ++i)
        t[i] = sdptr_->getScreenPos(i);

    AABB3d aabb(t[0], t[1], t[2]);
    aabb.clipAABB(box3d_);

    if (aabb.min.x >= aabb.max.x || aabb.min.y >= aabb.max.y)
        return;

    if (ShaderType::Frame == sdptr_->getType())
    {
        for (int i = 0; i < 3; ++i)
        {
            drawLine(t[i], t[(i + 1) % 3]);
        }
    }
    else
    {
        if (hzb_->rapidRefuseBox(aabb))
        {
            --info_.profile_.shaded_face_num_;
            return;
        }

        for (int y = aabb.min.y; y <= aabb.max.y; ++y)
        {
            for (int x = aabb.min.x; x <= aabb.max.x; ++x)
            {

                float depth = sdptr_->fragmentDepth(x, y);
                if (hzb_->finestZTest(x, y, depth))
                {
                    sdptr_->fragmentShader(x, y);
                    colorbuffer_->setPixel(x, y, sdptr_->getColor());
                }
            }
        }
    }
}

void Render::drawTriangleScanLine()
{
    ++info_.profile_.shaded_face_num_;

    if (ShaderType::Frame == sdptr_->getType())
    {
        AABB2d aabb;
        glm::vec3 t[3];
        for (int i = 0; i < 3; ++i)
            t[i] = sdptr_->getScreenPos(i);

        aabb.containTriangel(t[0], t[1], t[2]);
        aabb.clipAABB(box2d_);
        if (!aabb.valid)
            return;

        for (int i = 0; i < 3; ++i)
        {
            drawLine(t[i], t[(i + 1) % 3]);
        }
    }
    else
    {
        auto v0 = sdptr_->getVertices(0);
        auto v1 = sdptr_->getVertices(1);
        auto v2 = sdptr_->getVertices(2);
        std::vector<const Vertex *> vs = {v0, v1, v2};
        tri_scanliner_->scanConvert(vs);
    }
}

void Render::pipelineBegin()
{

    if (is_init_ == false)
    {
        std::cerr << "pipelineBegin: didn't use `pipelineInit` to initialize.\n";
        return;
    }
    // update the scene or render accorrding to the setting(modified by ImGui)
    if (info_.raster_setting_.scene_change == true)
    {
        loadDemoScene(info_.raster_setting_.scene_filename, info_.raster_setting_.shader_type);
        scene_.buildTLAS();
    }
    if (info_.raster_setting_.shader_change == true)
    {
        for (auto &inst : scene_.getAllInstances())
        {
            if (inst.shader_ != ShaderType::Light)
                inst.shader_ = info_.raster_setting_.shader_type;
        }
    }
    if (info_.raster_setting_.rasterize_change == true)
    {
        info_.rasterize_timer_.clear(); // each rasterize technique has different stages
    }
    if (info_.raster_setting_.leaf_num_change == true)
    {
        setBVHLeafSize(info_.raster_setting_.bvh_leaf_num);
        scene_.rebuildBLAS();
    }
    // update the camera if moved
    if (camera_.needUpdateView())
        updateViewMatrix();

    // prepare for shader
    sdptr_->bindCamera(std::make_shared<Camera>(camera_));
    sdptr_->bindLights(scene_.getLights());
    sdptr_->setFrustum(camera_.getNear(), camera_.getFar());

    // The pipeline happens here
    if (info_.raster_setting_.rasterize_type == RasterizeType::Bvh_hzb)
    {
        pipelineHZB_BVH();
    }
    else
    {
        pipelinePerInstance();
    }

    // show bvh structure
    if (info_.raster_setting_.show_tlas)
    {
        showTLAS();
    }
    else if (info_.raster_setting_.show_blas)
    {
        auto &asinstances = scene_.getAllInstances();
        for (auto &ins : asinstances)
            showBLAS(ins);
    }

    // calculate hzb_culled_face_num_
    info_.profile_.hzb_culled_face_num_ = info_.profile_.total_face_num_ - info_.profile_.shaded_face_num_ 
    - info_.profile_.back_culled_face_num_ - info_.profile_.clipped_face_num_;

    // this is for cmd user:
    // if(setting_.profile_report)
    //     printProfile();
}

void Render::pipelineGeometryPhase()
{

    auto &asinstances = scene_.getAllInstances();
    for (auto &ins : asinstances)
    {

        auto &obj = ins.blas_->object_;
        auto &mat_model = ins.modle_;

        // init vertex shader
        glm::mat4 mvp = mat_perspective_ * mat_view_ * mat_model;
        glm::mat4 normal_mat = glm::transpose(glm::inverse(mat_model));
        sdptr_->bindMVP(&mvp);
        sdptr_->bindViewport(&mat_viewport_);
        sdptr_->bindNormalMat(&normal_mat);
        sdptr_->bindModelMat(&mat_model);

        // MVP => clip space
        for (auto &v : obj->getVertices())
        {
            sdptr_->vertexShader(v);
        }

        // culling

        cullingTriangleInstance(ins, normal_mat);

        // clip space => NDC => screen space
        for (auto &newv : *ins.vertices_)
        {
            sdptr_->vertex2Screen(newv);
        }
    }
}

void Render::pipelinePerInstance()
{

#ifdef TIME_RECORD
    info_.rasterize_timer_.start("110.Geometry Phase");
#endif

    pipelineGeometryPhase();

#ifdef TIME_RECORD
    info_.rasterize_timer_.stop("110.Geometry Phase");
#endif

#ifdef TIME_RECORD
    info_.rasterize_timer_.start("120.Rasterize Phase(Per-Instance mode)");
#endif

    pipelineRasterizePhasePerInstance();

#ifdef TIME_RECORD
    info_.rasterize_timer_.stop("120.Rasterize Phase(Per-Instance mode)");
#endif
}

void Render::pipelineRasterizePhasePerInstance()
{

    // put each instance into the pipeline.
    auto &asinstances = scene_.getAllInstances();
    for (auto &ins : asinstances)
    {

        auto &obj = ins.blas_->object_;
        auto otype = obj->getPrimitiveType();
        auto &objmtls = obj->getMtls();

        auto &objvertices = *ins.vertices_;
        // auto& objmtlidx=*ins.mtlidx_;
        auto &primitive_buffer = *ins.primitives_buffer_;

        // init shader for the current instance
        sdptr_->setShaderType(ins.shader_);
        sdptr_->setPrimitiveType(otype);

        // for each primitive
        int face_cnt = 0;
        for (auto it = objvertices.begin(); it < objvertices.end(); face_cnt++)
        {

            Vertex *v1 = &(*it++);
            Vertex *v2 = &(*it++);
            Vertex *v3 = &(*it++);

            // assembly primitive
            sdptr_->assemblePrimitive(v1, v2, v3);

            // binds the material if it has one
            // int midx=objmtlidx[face_cnt];
            int midx = primitive_buffer[face_cnt].mtlidx_;
            if (midx >= 0 && midx < objmtls.size())
            {
                sdptr_->bindMaterial(objmtls[midx]);
            }
            else
            {
                sdptr_->bindMaterial(nullptr);
            }

            // render
            assert(otype == PrimitiveType::MESH);
            if (info_.raster_setting_.rasterize_type == RasterizeType::Easy_hzb)
                drawTriangleHZB();
            else if (info_.raster_setting_.rasterize_type == RasterizeType::Scan_convert)
                drawTriangleScanLine();
            else if (info_.raster_setting_.rasterize_type == RasterizeType::Naive)
                drawTriangleNaive();
            else
            {
                std::cerr << "unknown RasterizeType::setting_.rasterize_type\n";
                exit(-1);
            }

        } // end for-objvertices

    } // end of for-asinstances
}

/*
bool Render::mapWorldBox2ScreenBox(const BVHnode& node,const glm::mat4& transition,AABB3d& sbox){
    glm::vec3 bmin=node.bbox.min;
    glm::vec3 bmax=node.bbox.max;
    std::vector<glm::vec3> bboxpoints={bmin,{bmin.x,bmin.y,bmax.z},{bmin.x,bmax.y,bmax.z},{bmin.x,bmax.y,bmin.z},
                        {bmax.x,bmin.y,bmin.z},{bmax.x,bmin.y,bmax.z},                 bmax,{bmax.x,bmax.y,bmin.z}};

    AABB3d insbox;
    // find the screen space aabb3d
    for(auto& p:bboxpoints){
        auto newp=transition*glm::vec4(p,1);
        if(newp.w<=0){
            // part of the box is behind the camera
            return false;
        }
        p=mat_viewport_*(newp/newp.w);
        for(int i=0;i<3;++i){
            insbox.min[i]=std::min(insbox.min[i],p[i]);
            insbox.max[i]=std::max(insbox.max[i],p[i]);
        }
    }

    insbox.clipAABB(box3d_);

    // the box is invalid
    if(insbox.min.x>=insbox.max.x||insbox.min.y>=insbox.max.y){
        return false;
    }

    sbox=insbox;
    return true;
}
*/

void Render::pipelineHZB_BVH()
{

#ifdef TIME_RECORD
    info_.rasterize_timer_.start("110.Geometry Phase");
#endif

    pipelineGeometryPhase();

#ifdef TIME_RECORD
    info_.rasterize_timer_.stop("110.Geometry Phase");
#endif

#ifdef TIME_RECORD
    info_.rasterize_timer_.start("120.Rasterize Phase(HZB_BVH mode)");
#endif

    pipelineRasterizePhaseHZB_BVH();

#ifdef TIME_RECORD
    info_.rasterize_timer_.stop("120.Rasterize Phase(HZB_BVH mode)");
#endif
}

void Render::pipelineRasterizePhaseHZB_BVH()
{

#ifdef TIME_RECORD
    info_.rasterize_timer_.start("121.update SBox");
#endif
    // update bvh in screenspace
    auto &tlas = scene_.getTLAS();
    auto &tlas_tree = (*tlas.tree_);

    tlas.TLASupdateSBox();

#ifdef TIME_RECORD
    info_.rasterize_timer_.stop("121.update SBox");
#endif

#ifdef TIME_RECORD
    info_.rasterize_timer_.start("122.DfsTlas_BVHwithHZB()");
#endif
    auto &tlas_sboxes = *tlas.tlas_sboxes_;
    DfsTlas_BVHwithHZB(tlas_tree, tlas_sboxes, scene_.getAllInstances(), 0);

#ifdef TIME_RECORD
    info_.rasterize_timer_.stop("122.DfsTlas_BVHwithHZB()");
#endif
}

void Render::DfsTlas_BVHwithHZB(const std::vector<BVHnode> &tree, std::vector<AABB3d> &tlas_sboxes, const std::vector<ASInstance> &instances, uint32_t nodeIdx)
{

    if (nodeIdx >= tree.size())
    {
        std::cerr << "Render::DfsTlas_BVHwithHZB: nodeIdx>=tree.size()!\n";
        exit(-1);
    }

    auto &node = tree[nodeIdx];

    // IF the box is refused by HZB
    if (hzb_->rapidRefuseBox(tlas_sboxes[nodeIdx]))
    {
        return;
    }

    // ELSE dive deeper...
    if (node.left > 0 && node.right > 0)
    {
        // select a nearer node as a prior candidate
        AABB3d sbox_left = tlas_sboxes[node.left];
        AABB3d sbox_right = tlas_sboxes[node.right];
        if (sbox_left.min.z < sbox_right.min.z)
        {
            DfsTlas_BVHwithHZB(tree, tlas_sboxes, instances, node.left);
            DfsTlas_BVHwithHZB(tree, tlas_sboxes, instances, node.right);
        }
        else
        {
            DfsTlas_BVHwithHZB(tree, tlas_sboxes, instances, node.right);
            DfsTlas_BVHwithHZB(tree, tlas_sboxes, instances, node.left);
        }
    }
    else if (node.left == -1 && node.right == -1)
    {
        auto &inst = instances[node.prmitive_start];
        auto otype = inst.blas_->object_->getPrimitiveType();

        sdptr_->setShaderType(inst.shader_);
        sdptr_->setPrimitiveType(otype);
        assert(otype == PrimitiveType::MESH);

        DfsBlas_BVHwithHZB(inst, 0);
    }
    else if (node.left == -1 && node.right != -1)
    {
        DfsTlas_BVHwithHZB(tree, tlas_sboxes, instances, node.right);
    }
    else if (node.right == -1 && node.left != -1)
    {
        DfsTlas_BVHwithHZB(tree, tlas_sboxes, instances, node.left);
    }
}

void Render::DfsBlas_BVHwithHZB(const ASInstance &inst, int32_t nodeIdx)
{
    const std::vector<BVHnode> &tree = *inst.blas_->tree_;

    if (nodeIdx >= tree.size())
    {
        std::cerr << "Render::DfsTlas_BVHwithHZB: nodeIdx>=tree.size()!\n";
        exit(-1);
    }

    const BVHnode &node = tree[nodeIdx];

    // IF the box is refused by HZB
    if (hzb_->rapidRefuseBox(inst.blas_sboxes_->at(nodeIdx)))
    {
        return;
    }

    // ELSE dive deeper...
    if (node.left > 0 && node.right > 0)
    {
        // select a nearer node as the prior candidate
        AABB3d sbox_left = inst.blas_sboxes_->at(node.left);
        AABB3d sbox_right = inst.blas_sboxes_->at(node.right);
        if (sbox_left.min.z < sbox_right.min.z)
        {
            DfsBlas_BVHwithHZB(inst, node.left);
            DfsBlas_BVHwithHZB(inst, node.right);
        }
        else
        {
            DfsBlas_BVHwithHZB(inst, node.right);
            DfsBlas_BVHwithHZB(inst, node.left);
        }
    }
    else if (node.left == -1 && node.right == -1)
    {
        // reach the leaf: raseterize these triangles.

        int st_primitive = tree[nodeIdx].prmitive_start;

        for (int i = 0; i < tree[nodeIdx].primitive_num; ++i)
        {

            uint32_t face_idx = inst.blas_->primitives_indices_->at(st_primitive + i);
            auto &cur_face = inst.primitives_buffer_->at(face_idx);

            if (cur_face.clipflag_ == ClipFlag::clipped || cur_face.clipflag_ == ClipFlag::accecpted)
            {

                int32_t st_ver = cur_face.vertex_start_pos_;
                auto &instvertices = *inst.vertices_;
                auto &objmtls = inst.blas_->object_->getMtls();

                for (int v = 0; v < cur_face.vertex_num_; v += 3)
                { // >= 3 vertivces

                    Vertex *v1 = &instvertices[st_ver + v + 0];
                    Vertex *v2 = &instvertices[st_ver + v + 1];
                    Vertex *v3 = &instvertices[st_ver + v + 2];

                    // assembly primitive
                    sdptr_->assemblePrimitive(v1, v2, v3);

                    // binds the material if it has one
                    int midx = cur_face.mtlidx_;
                    if (midx >= 0 && midx < objmtls.size())
                    {
                        sdptr_->bindMaterial(objmtls[midx]);
                    }
                    else
                    {
                        sdptr_->bindMaterial(nullptr);
                    }

                    // render
                    drawTriangleHZB();
                }
            }
        }
    }
    else if (node.left == -1 && node.right != -1)
    {
        DfsBlas_BVHwithHZB(inst, node.right);
    }
    else if (node.right == -1 && node.left != -1)
    {
        DfsBlas_BVHwithHZB(inst, node.left);
    }
}

void Render::showTLAS()
{
    auto &tlas = scene_.getTLAS();
    auto &tlas_tree = (*tlas.tree_);
    if (tlas_tree.size())
    {
        traverseBVHandDraw(tlas_tree, 0, true);
    }
}

void Render::showBLAS(const ASInstance &inst)
{
    auto &blas = inst.blas_;
    auto &blas_tree = *(blas->tree_);
    if (blas_tree.size())
    {
        traverseBVHandDraw(blas_tree, 0, false, inst.modle_);
    }
}

/**
 * @brief traverse through BVH from top to bottom and draw each node's AABB.
 *
 * @param tree : the tree structure
 * @param nodeIdx : current node
 * @param is_TLAS : to support both TLAS and BLAS traversal, this on-off differentiates these two cases.
 * @param model   : specify the model matrix, which is necessary for BLAS without knowing its world position
 */
void Render::traverseBVHandDraw(const std::vector<BVHnode> &tree, uint32_t nodeIdx, bool is_TLAS, const glm::mat4 &model)
{
    if (nodeIdx >= tree.size())
    {
        std::cerr << "Render::traverseBVHandDraw:nodeIdx>=tree.size()!\n";
        exit(-1);
    }
    auto &node = tree[nodeIdx];

    glm::vec3 bmin = node.bbox.min;
    glm::vec3 bmax = node.bbox.max;
    glm::vec3 bcenter = (bmin + bmax) * 0.5f;
    std::vector<glm::vec3> bboxpoints = {bmin, {bmin.x, bmin.y, bmax.z}, {bmin.x, bmax.y, bmax.z}, {bmin.x, bmax.y, bmin.z}, {bmax.x, bmin.y, bmin.z}, {bmax.x, bmin.y, bmax.z}, bmax, {bmax.x, bmax.y, bmin.z}};
    // transform p*v=>/z=>viewport
    bool valid[12] = {true};
    glm::mat4 trans = is_TLAS ? (mat_perspective_ * mat_view_) : (mat_perspective_ * mat_view_ * model);

    bcenter = trans * glm::vec4(bcenter, 1.0f);

    for (int i = 0; i < bboxpoints.size(); ++i)
    {
        auto &p = bboxpoints[i];
        glm::vec4 pos = trans * glm::vec4(p, 1.0f);
        int bias[3] = {0};
        for (int t = 0; t < 3; ++t)
        {
            if (pos[t] < bias[t])
                pos[t] -= 1e-3;
            else
                pos[t] += 1e-3;
        }

        if (pos.w > 1e-6)
        {
            pos /= pos.w;
            pos = mat_viewport_ * pos;
            p = glm::vec3(pos);
            valid[i] = true;
        }
        else
        {
            valid[i] = false;
        }
    }
    // draw 12 lines
    glm::vec4 color = is_TLAS ? glm::vec4(255.0) : glm::vec4(0.0, 100.0, 100.0, 1.0);
    if (info_.raster_setting_.show_tlas && is_TLAS || info_.raster_setting_.show_blas && !is_TLAS)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (valid[i] && valid[(i + 1) % 4])
                drawLine3d(bboxpoints[i], bboxpoints[(i + 1) % 4], color);
            if (valid[i] && valid[i + 4])
                drawLine3d(bboxpoints[i], bboxpoints[i + 4], color);
            if (valid[i + 4] && valid[(i + 1) % 4 + 4])
                drawLine3d(bboxpoints[i + 4], bboxpoints[(i + 1) % 4 + 4], color);
        }
    }

    // dive into blas if possible and required
    if (is_TLAS && node.left == -1 && node.right == -1)
    {
        if (info_.raster_setting_.show_blas)
        {
            auto &tlas = scene_.getTLAS();
            uint32_t idx = node.prmitive_start;
            showBLAS(tlas.all_instances_[idx]);
        }
    }

    if (node.left != -1)
        traverseBVHandDraw(tree, node.left, is_TLAS, model);
    if (node.right != -1)
        traverseBVHandDraw(tree, node.right, is_TLAS, model);
}

// drawLine in screen space
void Render::drawLine3d(glm::vec3 t1, glm::vec3 t2, const glm::vec4 &color)
{
    // naive clip: create a aabb2d to intersect with the screen
    AABB2d aabb;
    aabb.containLine(glm::vec2(t1), glm::vec2(t2));
    aabb.clipAABB(box2d_);

    // make sure: x-axis is less steep and t1 is the left point
    bool swap_flag = 0;
    if (std::abs(t1.x - t2.x) < std::abs(t1.y - t2.y))
    {
        std::swap(t1.x, t1.y);
        std::swap(t2.x, t2.y);
        swap_flag = 1;
    }
    if (t1.x > t2.x)
    {
        std::swap(t1, t2);
    }
    bool positive_flag = 1;
    int dx = t2.x - t1.x;
    int dy = t2.y - t1.y;
    if (dy < 0)
    {
        dy = -dy;
        positive_flag = 0;
    }

    int delta2 = dy * 2;
    int error2 = 0;
    int y = t1.y;

    auto &depthbuf = (info_.raster_setting_.rasterize_type == RasterizeType::Easy_hzb || info_.raster_setting_.rasterize_type == RasterizeType::Bvh_hzb)
                         ? hzb_->getFinesetZbuffer()
                         : *zbuffer_;

    float inv_t1z = 1.0 / t1.z;
    float inv_t2z = 1.0 / t2.z;

    for (int x = t1.x; x <= t2.x; ++x)
    {
        float dt = (x - t1.x) / float(dx);
        // float curz=(1-dt)*t1.z+dt*(t2.z);
        float curz = 1.0 / ((1 - dt) * inv_t1z + dt * inv_t2z);
        if (swap_flag)
        {
            if (x <= aabb.max.y && x >= aabb.min.y && y <= aabb.max.x && y >= aabb.min.x)
            {
                float curdepth = depthbuf.getDepth(y, x);
                if (curz <= curdepth + 10 * srender::EPSILON)
                {
                    colorbuffer_->setPixel(y, x, color);
                    depthbuf.setDepth(y, x, curz);
                }
            }
        }
        else
        {
            if (y <= aabb.max.y && y >= aabb.min.y && x <= aabb.max.x && x >= aabb.min.x)
            {
                float curdepth = depthbuf.getDepth(x, y);
                if (curz <= curdepth + 10 * srender::EPSILON)
                {
                    colorbuffer_->setPixel(x, y, color);
                    depthbuf.setDepth(x, y, curz);
                }
            }
        }
        error2 += delta2;
        if (error2 > dx)
        {
            y += positive_flag ? 1 : -1;
            error2 -= dx * 2;
        }
    }
}

// drawPoint in screen space with specified radius
void Render::drawPoint(const glm::vec2 p, float radius, const glm::vec4 color)
{
    int center_x = static_cast<int>(std::round(p.x));
    int center_y = static_cast<int>(std::round(p.y));

    float radius_sq = radius * radius;
    glm::vec2 minp = p - radius;
    glm::vec2 maxp = p + radius;
    AABB2d box;
    box.containLine(minp, maxp);
    box.clipAABB(box2d_);

    for (int y = box.min.y; y <= box.max.y; ++y)
    {
        for (int x = box.min.x; x <= box.max.x; ++x)
        {

            float dx = static_cast<float>(x) - p.x;
            float dy = static_cast<float>(y) - p.y;
            float distance_sq = dx * dx + dy * dy;

            if (distance_sq <= radius_sq)
            {
                colorbuffer_->setPixel(x, y, color);
            }
        }
    }
}

/**
 * @brief update members accrordingly after camera's update
 */
void Render::afterCameraUpdate()
{
    colorbuffer_->reSetBuffer(camera_.getImageWidth(), camera_.getImageHeight());
    zbuffer_->reSetBuffer(camera_.getImageWidth(), camera_.getImageHeight());
    updateMatrix();

    box2d_.min = {0, 0};
    box2d_.max = {camera_.getImageWidth() - 1, camera_.getImageHeight() - 1};
    box3d_.min = {0, 0, -1};
    box3d_.max = {camera_.getImageWidth() - 1, camera_.getImageHeight() - 1, 1};
}

void Render::printProfile()
{

    std::cout << "-------------- face counter  -----------------\n";
    std::cout << "total         =" << info_.profile_.total_face_num_ << std::endl;
    std::cout << "shaded        =" << info_.profile_.shaded_face_num_ << std::endl;
    std::cout << "back_culled   =" << info_.profile_.back_culled_face_num_ << std::endl;
    std::cout << "clipped       =" << info_.profile_.clipped_face_num_ << std::endl;
    std::cout << "hzb_culled    =" << info_.profile_.hzb_culled_face_num_ << std::endl;
}

void Render::initRenderIoInfo()
{
    info_.begin_path_tracing=false;
    info_.profile_report = true;

    // rasterizer
    info_.raster_setting_.scene_filename = "veach-mis";// cornell-box // veach-mis
    info_.raster_setting_.bvh_leaf_num = 12;
    info_.raster_setting_.back_culling = true;
    info_.raster_setting_.earlyz_test = true;
    info_.raster_setting_.rasterize_type = RasterizeType::Naive;
    info_.raster_setting_.show_tlas = false;
    info_.raster_setting_.show_blas = false;
    info_.raster_setting_.shader_type = ShaderType::Depth;

    // path tracer
    info_.tracer_setting_.filename_=info_.raster_setting_.scene_filename;
    info_.tracer_setting_.filepath_="./";
    info_.tracer_setting_.max_depth_=2;
    info_.tracer_setting_.spp_=1;
    info_.tracer_setting_.tiles_num_=1;
}

/**
 * @brief this is the top interface for camera setting, each scene must use this function to initial its camera.
 */
void Render::setCamera(glm::vec3 pos, glm::vec3 lookat, glm::vec3 right, float fov, float ratio, int image_width, float near, float far)
{
    if (image_width != camera_.getImageWidth() || image_width / ratio != camera_.getImageHeight())
        resize_viewport_flag_ = true;

    right = glm::normalize(right);
    camera_.setCameraPos(pos, lookat, right);
    camera_.setFrustrum(fov, near, far);
    camera_.setViewport(image_width, ratio);

    afterCameraUpdate();
}

/**
 * @brief after rasterization of a frame, some cleanning work is handled here.
 */
void Render::cleanFrame()
{
    colorbuffer_->clear();
    zbuffer_->clear();
    if (info_.raster_setting_.rasterize_type == RasterizeType::Easy_hzb || info_.raster_setting_.rasterize_type == RasterizeType::Bvh_hzb)
        hzb_->clear();

    if (info_.profile_report)
    {
        info_.profile_.clear();
    }

    info_.rasterize_timer_.reset();
}