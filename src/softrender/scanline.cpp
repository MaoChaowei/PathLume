#include"scanline.h"

using namespace ScanLine;

EdgeNode::EdgeNode(const Vertex* top,const Vertex* down,bool step):top_vertex_(top),down_vertex_(down),init_step_y_(step){
    x_=top->s_pos_.x;
    float dy=floor(top_vertex_->s_pos_.y)-floor(down_vertex_->s_pos_.y);
    //float dy=top_vertex_->s_pos_.y-down_vertex_->s_pos_.y;
    yspan_=(int)(top_vertex_->s_pos_.y)-(int)down_vertex_->s_pos_.y + 1;

    // if(dy<1-srender::EPSILON)
    //     horizontal_=true;
    // else
    //     horizontal_=false;


    float one_over_z1 = 1.0f / top_vertex_->s_pos_.z;
    float one_over_w1 = 1.0f / top_vertex_->c_pos_.w;
    glm::vec3 norm1_w1=top_vertex_->w_norm_*one_over_w1;
    glm::vec4 color1_w1=top_vertex_->color_*one_over_w1;
    glm::vec2 uv1_w1=top_vertex_->uv_*one_over_w1;
    glm::vec3 wpos1_w1=top_vertex_->w_pos_*one_over_w1;
    
    inv_z_=one_over_z1;
    inv_w_=one_over_w1;
    norm_w_=norm1_w1;
    color_w_=color1_w1;
    uv_w_=uv1_w1;
    wpos_w_=wpos1_w1;

    // if(!horizontal_){
        float one_over_z2 = 1.0f / down_vertex_->s_pos_.z;
        float one_over_w2 = 1.0f / down_vertex_->c_pos_.w;
        glm::vec3 norm2_w2=down_vertex_->w_norm_*one_over_w2;
        glm::vec4 color2_w2=down_vertex_->color_*one_over_w2;
        glm::vec2 uv2_w2=down_vertex_->uv_*one_over_w2;
        glm::vec3 wpos2_w2=down_vertex_->w_pos_*one_over_w2;

        float inv_dy=1.0/dy;
        dx_dy_=(down_vertex_->s_pos_.x-top_vertex_->s_pos_.x)*inv_dy;

        dinvz_dy_=(one_over_z2-one_over_z1)*inv_dy;
        dinvw_dy_=(one_over_w2-one_over_w1)*inv_dy;
        dnorm_w_dy_=(norm2_w2-norm1_w1)*inv_dy;
        dcol_w_dy_=(color2_w2-color1_w1)*inv_dy;
        duv_w_dy_=(uv2_w2-uv1_w1)*inv_dy;
        dwpos_w_dy_=(wpos2_w2-wpos1_w1)*inv_dy;

        if(step)
            stepY();
    // }

}

void EdgeNode::normalizeAttributes() {
    norm_w_ = glm::normalize(norm_w_);
    color_w_ = glm::clamp(color_w_, glm::vec4(0.0f), glm::vec4(255.0f));
}

void EdgeNode::stepY(){
    // if(!horizontal_){
        x_+=dx_dy_;
        inv_z_+=dinvz_dy_;
        inv_w_+=dinvw_dy_;
        norm_w_+=dnorm_w_dy_;
        color_w_+=dcol_w_dy_;
        uv_w_+=duv_w_dy_;
        wpos_w_+=dwpos_w_dy_;

        normalizeAttributes();
    // }else{
    //     x_=down_vertex_->s_pos_.x;
    //     inv_z_ = 1.0f / down_vertex_->s_pos_.z;
    //     inv_w_ = 1.0f / down_vertex_->c_pos_.w;
    //     norm_w_=down_vertex_->w_norm_*inv_w_;
    //     color_w_=down_vertex_->color_*inv_w_;
    //     uv_w_=down_vertex_->uv_*inv_w_;
    //     wpos_w_=down_vertex_->w_pos_*inv_w_;
    // }
    --yspan_;   
}



PerTriangleScanLiner::PerTriangleScanLiner(const AABB2d& box,std::shared_ptr<ColorBuffer>& fb,std::shared_ptr<DepthBuffer>& zb,std::shared_ptr<Shader>& sp) {
    screen_box_=box;
    fbuffer_=fb;
    zbuffer_=zb;
    sdptr_=sp;
    idx2ET_.resize(box.max.y+1);
    idx2PT_.resize(box.max.y+1);
    init_=true;
}

void PerTriangleScanLiner::reFresh(){
    ET_.clear();
    AET_.clear();
    for(auto& v:idx2ET_)
        v.clear();
}

void PerTriangleScanLiner::scanConvert(std::vector<const Vertex*>& vs){
    if(!init_){
        std::cerr<<"PerTriangleScanLiner::scanConvert : fail to pass 'if(!zbuffer_||!fbuffer_||!sdptr_)'\n";
        exit(-1);
    }
    assert(vs.size()==3);
    reFresh();
    AABB2d bbox;
    bbox.containTriangel(vs[0]->s_pos_,vs[1]->s_pos_,vs[2]->s_pos_);
    bbox.clipAABB(screen_box_);
    if(!bbox.valid)
        return;

    buildET(vs);

    for(int y=bbox.max.y;y>=bbox.min.y;--y){
        // add edge if there is any
        auto& newedges=idx2ET_[y];
        for(auto idx:newedges)
            AET_.emplace_back(&ET_[idx]);

        // update AET
        int validCount = 0;
        for (size_t i = 0; i < AET_.size(); i++) {
            if (AET_[i]->yspan_ > 0) {
                AET_[validCount++] = AET_[i];
            }
        }
        AET_.resize(validCount); 

        if(AET_.empty()) 
            continue;

        assert(AET_.size()==2);// because there is only one triangle
        
        EdgeNode* left=AET_[0];
        EdgeNode* right=AET_[1];
        if(left->x_>right->x_ || (left->x_==right->x_&&left->dx_dy_>right->dx_dy_) ) 
            std::swap(left,right);

        // interpolate and render
        rasterizeScanline(y,left,right);

        // update Edgenode
        left->stepY();
        right->stepY();            
    }

}

void PerTriangleScanLiner::rasterizeScanline(int y,const EdgeNode* left,const EdgeNode* right){
    float inv_dx=1.0/(right->x_-left->x_);

    float dinvz_dx=(right->inv_z_-left->inv_z_)*inv_dx;
    float dinvw_dx=(right->inv_w_-left->inv_w_)*inv_dx;
    auto dnorm_w_dx=(right->norm_w_-left->norm_w_)*inv_dx;
    auto duv_w_dx=(right->uv_w_-left->uv_w_)*inv_dx;
    auto dcol_w_dx=(right->color_w_-left->color_w_)*inv_dx;
    auto dwpos_w_dx=(right->wpos_w_-left->wpos_w_)*inv_dx;


    float invz=left->inv_z_;
    float invw=left->inv_w_;
    auto norm_w=left->norm_w_;
    auto uv_w=left->uv_w_;
    auto col_w=left->color_w_;
    auto wpos_w=left->wpos_w_;
    int rightX=std::min(right->x_,screen_box_.max.x);
    int leftX=std::max(left->x_,screen_box_.min.x);

    for(int x=leftX;x<=rightX;++x){
        float z=1.0/invz;
        float w=1.0/invw;
        auto wnorm=norm_w*w;
        auto uv=uv_w*w;
        auto col=col_w*w;
        auto wpos=wpos_w*w;
        wnorm = glm::normalize(wnorm);
        col = glm::clamp(col, glm::vec4(0.0f), glm::vec4(255.0f));
        if(zbuffer_->zTest(x,y,z)==true){
            // interpolate and caculate color.
            FragmentHolder fragment(x,y,z,col,uv,wpos,wnorm);
            sdptr_->fragmentShader(fragment);
            fbuffer_->setPixel(x,y,sdptr_->getColor());
        }

        invz+=dinvz_dx;
        invw+=dinvw_dx;
        norm_w+=dnorm_w_dx;
        uv_w+=duv_w_dx;
        col_w+=dcol_w_dx;
        wpos_w+=dwpos_w_dx;
    }

}

bool PerTriangleScanLiner::buildET(std::vector<const Vertex*>& vs){
    std::sort(vs.begin(),vs.end(),[](const Vertex* a,const Vertex* b){
        return a->s_pos_.y > b->s_pos_.y;
    });

    // degenerated triangle
    bool horizon_02=((int)vs[0]->s_pos_.y==(int)vs[2]->s_pos_.y);
    if(horizon_02)// invalid triangle
        return false;
    
    bool horizon_01=((int)vs[0]->s_pos_.y==(int)vs[1]->s_pos_.y);
    bool horizon_12=((int)vs[1]->s_pos_.y==(int)vs[2]->s_pos_.y);

    int y0=(int)vs[0]->s_pos_.y;
    int y1=(int)vs[1]->s_pos_.y;
    int y2=(int)vs[2]->s_pos_.y;

    // case 1: non horizontal
    if(!(horizon_01||horizon_12)){
        // v0-v1
        ET_.emplace_back(vs[0],vs[1],false);
        idx2ET_.at(y0).emplace_back(ET_.size()-1);
        // v0-v2
        ET_.emplace_back(vs[0],vs[2],false);
        idx2ET_.at(y0).emplace_back(ET_.size()-1);
        // v1-v2
        ET_.emplace_back(vs[1],vs[2],true);
        idx2ET_.at(std::max(y1-1,0)).emplace_back(ET_.size()-1);       // start from y1-1,because y1 has been considered in v0-v1
    }
    // case 2: upper edge horizontal
    else if(horizon_01){
        // v0-v2
        ET_.emplace_back(vs[0],vs[2],false);
        idx2ET_.at(y0).emplace_back(ET_.size()-1);
        // v1-v2
        ET_.emplace_back(vs[1],vs[2],false);
        idx2ET_.at(y0).emplace_back(ET_.size()-1);
    }
    // case 3: lower edge horizontal
    else{
        ET_.emplace_back(vs[0],vs[1],false);
        idx2ET_.at(y0).emplace_back(ET_.size()-1);
        ET_.emplace_back(vs[0],vs[2],false);
        idx2ET_.at(y0).emplace_back(ET_.size()-1);
    }

    return true;
}