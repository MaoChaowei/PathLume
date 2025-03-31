# PathLume

# 1 项目概述

该项目从底层实现了一个基于蒙特卡洛的路径追踪器。具体功能包括：

- 实现基于蒙特卡洛的无偏路径追踪算法。采用基于BRDF的重要性采样、多重重要性采样，以及低差异性序列等技术，加速收敛和降低方差。通过基于表面积启发的方式构建BVH加速结构，并采用双层加速结构管理场景。借助多线程技术实现了并行渲染，并通过交互窗口实时更新光追的渲染过程。
- 实现Lambert漫反射、Blinn-phong镜面反射和完全镜面反射等BRDF模型。
- 交互窗口为自主开发的基于cpu的光栅器，能够支持实时的更新和交互，详细内容可见该项目主页 https://github.com/MaoChaowei/TinySoftRasterizer

## 项目界面及交互说明

- 加载场景后的光栅化渲染界面，可通过深度图和法向图观察模型，通过“WASD”移动相机，通过鼠标左键拖拽改变相机朝向。

![image.png](assets/doc/img/image.png)

![image.png](assets/doc/img/image%201.png)

- 下方交互栏中“Path Tracing Setting”是与路径追踪相关的设置，包括
    - Max Depth——最大路径，0表示无穷（RR截断），1表示直接光照…
    - Tiles Size——屏幕空间的分块大小，表示将屏幕划分为n*n个区域，并行渲染。虽然最多可以分为16\*16个区域，但实际最大线程并发数不超过机器的逻辑线程数。
    - Sample per Pixel——即每像素的样本数。
    - Light Split——即每个交点在做直接光照计算时考虑多少条shadow ray。
    - Begin Path tracing——点击即可在“当前视角”和“当前配置”下进入路径追踪渲染模式。渲染期间，所有交互将不可用。直到渲染结束，程序会自动保存渲染结果。之后取消Begin Path tracing，将恢复到软光栅管线。
    - Render Time——上次渲染的总时间（单位，秒）。

- 关于场景文件的输入：由于时间关系，目前的做法是将场景的定义写死在demoscene.cpp中（之后将采用xml文件的方式将场景定义完全交给用户）。目前无法绕过代码直接修改场景配置，我在代码中提供了四个实例场景，可以通过交互栏的下拉框“Demo Scene”进行选择。
  
## 项目结构

主要文件的作用说明如下。

```markdown
PATHLUME
├── assets                    // 模型文件和图片等资产
├── build                     // 工程文件等(after compilation)
├── external                  // 第三方库
├── src                       // 项目源代码
│   ├── common                // 场景等公共结构、工具函数等
|   │   └── ...
│   ├── softrender            // 软光栅器的实现
|   ├── pathtracer            // 光线追踪的实现
│   │   ├── bsdf.h            // 定义材质的双向反射分布函数的采样和取值
│   │   ├── emitter.h         // 定义发光面的采样
│   │   ├── film.h            // 定义胶片，即成像平面
│   │   ├── tile.h            // 每一个tile由一个线程运行，负责一块区域的独立渲染
│   │   ├── hitem.h           // 定义三角形、AABB盒等可交物体的求交算法
│   │   ├── sample.h          // 定义采样器，包括分层采样等方法
│   │   ├── pathtracer.h      // 定义路径追踪的实际执行算法
│   │   ├── ray               // 定义光线
│   │   └── *.cpp
│   ├── main.cpp
├── CMakeLists.txt
├── setup_debug.bat
└── setup_release.bat
```
## 开发环境说明

- 操作系统： windows11
- cpu型号： intel core i9
- 编译链：camke项目，支持多种编译链。本人开发中采用mingw64 v14.2.0版本的gcc和g++编译。
- 第三方库支持（见external文件夹）
    - glfw、glad、opengl：提供窗口构建和显示渲染，注意仅仅借助opengl将帧缓存绑定到矩形纹理中，最后渲染到窗口的viewport上，软光栅器未采用任何图形学接口；
    - glm：提供高效的矩阵和向量运算；
    - imgui: 通过即时渲染模式提供非常轻量和高效简易的图形界面，用来构建交互界面；
    - stb_image: 读取纹理数据的轻量库；
    - tiny_obj_loader: 读取obj格式模型的轻量库；

# 2 编译与运行
需要先配置cmake环境，然后采用合适的编译链进行编译。建议**在Release**模式下运行，在cmake指令后添加`-DCMAKE_BUILD_TYPE=Rlease`即可。

- windows

项目根目录下提供了windows的构建编译的脚本文件，直接在cmd窗口等windows终端中执行该脚本文件即可：

```cpp
cd path/to/project root
setup_release.bat          // 执行cmake等指令，需要先安装cmake环境
build_release/pathlume     // 执行程序
```

- linux

```cpp
cd path/to/project_root
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release  -DCMAKE_C_COMPILER=/path/to/gcc -DCMAKE_CXX_COMPILER=/path/to/g++    ..
cmake --build .      // or: make ..
build/pathlume
```

看到build文件夹中生成了目标可执行程序即编译成功。运行结果如下：

- case 1:spp=200, Depth=16.
![cornell-box_S200_L1_D16_T5919.335938_C31.png](assets/doc/img/cornell-box_S200_L1_D16_T5919.335938_C31.png)
- case 2:spp=800, Depth=10.
![veach-mis_S800_L1_D10_T4538.038086_C31.png](assets/doc/img/veach-mis_S800_L1_D10_T4538.038086_C31.png)
- case 3：spp=800, Depth=5.
![bathroom2_S800_L1_D5_T7108.475586_C31.png](assets/doc/img/bathroom2_S800_L1_D5_T7108.475586_C31.png)

# 3 架构与实现
光栅化及窗口管理部分基本结构如图所示，本项目聚焦于光线追踪，所以不加赘述。
![img/1.png](assets/doc/img/1.png)
下面重点介绍光追的架构和本项目的工程实现方法。

## 基本流程及多线程并发说明
- 函数`void Render::GameLoop()`是主要流程，实现窗口的不断刷新和交互处理。当检测到要进行光线追踪算法时，函数将用户的GUI输入交与路径追踪管线，并暂停光栅管线。并将启动子线程独立进行后台的渲染工作，父线程继续刷新viewport：
```c++
    std::thread rtwork(&Render::startPathTracer,this);
    rtwork.detach();
```
- 函数`void Render::startPathTracer()`中首先会根据用户的GUI输入，创建film结构和并行渲染的tiles结构；然后在场景中寻找所有发光面，便于后续光源采样；随后file结构执行并行渲染算法。
```c++
    // 1.create film and tiles
    std::shared_ptr<Film> film=camera_.getNewFilm();
    film->initTiles(info_.tracer_setting_,colorbuffer_,&scene_);
    // 2.make sure: world position and emitters are prepared
    scene_.findAllEmitters();
    ...
    // 3. render
    int thread_num=film->parallelTiles();
    ...
```
- 函数`int Film::parallelTiles()`的主要工作是并行启动各个tile的渲染函数。每个tile都管理“互不重叠的图像区域”，并有相同的render方法，负责计算所管理像素的颜色。代码如下所示。render方法主要通过三层循环反复调用路径追踪算法：`tracer_->Li(ray,pRec)`，其中tracer_成员是tile对象通过共享指针绑定的一个Pathtracer对象，负责执行真正的路径追踪算法。
```c++
void Tile::render(){

    info_.avg_length=0;

    for(int j=0;j<pixels_num_.y;++j){
        for(int i=0;i<pixels_num_.x;++i){
            glm::vec3 color(0);
            sampler_->startPixle();
            for(int s=0;s<setting_.spp_;++s){
                // generate a ray
                glm::vec2 offset=sampler_->getSample2D();
                glm::vec3 origin=film_->camera_pos_;
                glm::vec3 sample_pos=up_lt_pos_+float(i+offset.x)*film_->deltaX_+float(j+offset.y)*film_->deltaY_;
                glm::vec3 direction=sample_pos-origin;
                float startT=srender::EPSILON;
                float endT=srender::MAXFLOAT;

                Ray ray(origin,direction,startT,endT);
                // trace the ray and get its color
                PathTraceRecord pRec(*scene_,*sampler_,setting_.light_split_);
                color+=tracer_->Li(ray,pRec);
                info_.avg_length+=pRec.curdepth;
                
                // move on to the next image sample.
                sampler_->nextPixleSample();

            }

            // set color to buffer
            color=(float)(1.0/setting_.spp_)*color;
            setPixel(i,j,glm::vec4(color,1.0));

        }
    }

    // do some statistics
    info_.avg_length/=(pixels_num_.x*pixels_num_.y*setting_.spp_);

}
```

## 蒙特卡洛路径追踪算法
PathTracer类是路径追踪的基类，其中`std::shared_ptr<IntersectRecord> traceRay(const Ray& ray,const Scene* scene)`方法实现光线与场景的求交，并返回求交结果，而`virtual glm::vec3 Li(const Ray ray,PathTraceRecord& pRecord)`方法是留给派生类实现的着色函数，表示求解光线与场景第一个交点的radiance rgb。

项目中实践了NEE和MIS两种Pathtracer，它们的关系和差异总结如下：
1. **NEE（Next Event Estimation）**
    - 又称“下一事件估计”或“直接光照采样”。
    - 在路径追踪的每一次弹射（或在某些实现中只在第一次弹射）时，额外对场景中的光源进行显式采样，从而估计直接光照的贡献。
    - 这样可以有效避免**只依赖漫反射或光滑表面的 BSDF 方向**去采样光源时出现的高方差。例如在明暗对比强烈、光源较小且表面较高光泽（接近镜面）的场合，仅依赖 BSDF 采样往往会错过重要方向而带来噪声。
    - 但如果只做 NEE，也就是只做“光源采样”，而所有后续弹射方向仍然只由 BSDF 随机产生，对于高光泽表面也可能依然会有分布不匹配的问题；或者在某些几何复杂场景下，光源采样容易被遮挡，等等。具体效果需要结合场景而定。
2. **MIS（Multiple Importance Sampling）**
    - 即“多重重要性采样”，将BSDF 采样与光源采样（NEE）两种策略结合起来。
    - 在估计直接光照时，每个 bounce（弹射）既会使用 BSDF 方向采样去做一次光线追踪，又会使用光源采样去直接连线光源，然后用 MIS 的加权公式（如 balance heuristic）进行合并，最大程度地利用各自的长处并降低方差。
    - 这样可以在**镜面或高光材质**（更适合 BSDF 采样）以及**大面积光源或可见度高的光源**（更适合光源采样）等不同场景下，都取得相对稳定、均衡的采样效果。
    - 因此，MIS 通常被认为是通用性最强、噪点最少的方法之一。  
  
下面重点介绍MIS算法的实现
  
根据渲染方程，一个点的出射辐亮度可以认为由这三部分组成：**自发光、直接光照、间接光照。** 下面说明每一部分的计算。

- **自发光项Le**：当找到交点后，判断交点的材质，如果为发光材质，则将其辐亮度作为Le项。为保证能量守恒，只能在第一次碰到光源时考虑一次自发光，即光线在深度为1时才考虑Le。
- **直接光照Ld**：在NEE算法中，我们根据面积微元dA和立体角微元dw的关系转换积分域，从对光源的微分立体角采样转变为直接在光源表面采样，而在大光源的镜面反射中，这种采样得到的出射方向大概率与brdf的lobe集中的方向不同，导致大量光线浪费。MIS则采用分别对光源和brdf采样，通过平衡启发权重来保证两种采样的无偏，其本质是想要借助两种采样和权重系数来形成一种能与bsdf*Li形状类似的pdf，从而加速收敛。在我的代码中，直接光照采样后只需立即求解bsdf采样同方向的pdf值，然后用veach论文中的平方式权重函数求解该light的pdf和bsdf的pdf的权重系数，作为对直接光照采样的权重。接下来对bsdf采样，如果采样方向没有碰到光源，则bsdf采样项的权重为1，直接继续追踪这条光线，如果碰到了光源则求解bsdf采样项的权重，加权后直接结束这条光线。
- **间接光照Li**：直接光照的对bsdf采样时已经生成了新的光线，所以直接用该光线作为下一光线即可。为了调试的方便，我采用循环的方式实现光线追踪，用一个系数throughput来累乘地记录bsdf*cos/pdf项。在俄罗斯轮盘赌（RR）时，也通过throughput的值来动态的调整RR的概率，throughput越小，则截断该光线的概率越大，因为throughput太小会导致下一交点的直接光照并不能提供太大的影响，我们倾向于不继续行进。

核心代码如下：
```c++
glm::vec3 MonteCarloPathTracer::Li(const Ray ray,PathTraceRecord& pRecord){

    const Scene& scene=pRecord.scene;
    Sampler& sampler=pRecord.sampler;

    // For turning recursion into loop, `throughput` multiplies bsdf*cos/pdf each bounce.
    glm::vec3 throughput(1.0);
    // record the radiance along the path
    glm::vec3 radiance(0.f);
    // record the current ray
    Ray curRay(ray);
    // Trace the current ray
    std::shared_ptr<IntersectRecord> inst=traceRay(curRay,&scene);
    if(!inst){
        radiance+=glm::vec3(0.f);// could be an environment map 
        return radiance;
    }

    // Start Path Tracing!
    while((pRecord.curdepth++)<max_depth_||max_depth_<=0){

        auto& mtl=inst->material_;
        if(!mtl){
            throw std::runtime_error("Li(const Ray ray,PathTraceRecord& pRecord): the hit point doesn't own a material!");
        }

        /*-----------------------  0.EMISSION ------------------------*/
        // for the (1)First hit with the (2)Front face of an (3)Emitter, get radiance
        bool is_emitter=(bool)(mtl->type_&MtlType::Emissive);
        if( is_emitter
            &&glm::dot(curRay.dir_,inst->normal_)<0.f
            &&pRecord.curdepth==1)
        {
            radiance+=throughput*mtl->getEmit();
        }

        //-----------------------------------------------------------//
        /*--------------------- 1.DIRECT LIGHT ----------------------*/
        //-----------------------------------------------------------//
        float u=sampler.pcgRNG_.nextFloat();
        auto bsdf=inst->getBSDF(u); // pick a bsdf among all possible bsdfs


        /* --------- MIS: Sample Light's PDF ----------*/

        glm::vec3 direct(0.f);
        int t=pRecord.light_split;

        while(!is_emitter&&t--){
            LightSampleRecord lsRec;
            glm::vec3 adjust_pos=inst->pos_+inst->normal_*(float)(0.001);    //prevent from self-intersection
            scene.sampleEmitters(adjust_pos,lsRec,sampler);  

            // Trace a Shadow Ray
            if(lsRec.shadow_ray_){ 

                std::shared_ptr<IntersectRecord> light_inst=traceRay(*lsRec.shadow_ray_,&scene);
                // if visible, update radiance
                if(light_inst&&fabs(light_inst->t_-lsRec.dist_)<lsRec.dist_*0.01){
                    
                    glm::vec3 wo=inst->ray2TangentSpace(-curRay.dir_);
                    glm::vec3 wi=inst->ray2TangentSpace(lsRec.shadow_ray_->dir_);

                    BSDFRecord bsdfRec(*inst,sampler,wo,wi);
                    bsdf->evalBSDF(bsdfRec);        
                    float cosTheta = std::max(0.f, wi.z);

                    bool need_mis=needMIS(bsdf->bsdf_type_);
                    float weight=need_mis?getMISweight(lsRec.pdf_,bsdfRec.pdf):1.0;

                    direct+=throughput*bsdfRec.bsdf_val*lsRec.value_*cosTheta*weight;
                }
            }
        }

        radiance+=direct/float(pRecord.light_split);
        
        /* --------- MIS: Sample BRDF's PDF ----------*/

        BSDFRecord bsdfRec(*inst,sampler,-curRay.dir_);
        bsdf->sampleBSDF(bsdfRec);
        bsdf->evalBSDF(bsdfRec);
        if(!bsdfRec.isValid())// If bsdf value or pdf is too small, this path would gain us little benefit. 
            break;
        
        // generate next direction and trace it
        glm::vec3 wi_world=inst->wi2WorldSpace(bsdfRec.wi);
        curRay=Ray(inst->pos_+inst->normal_*0.001f,wi_world);
        inst=traceRay(curRay,&scene);
        if(!inst)
            break;
        
        // update throughput (recursion)
        throughput*=bsdfRec.bsdf_val*bsdfRec.costheta/bsdfRec.pdf;


        
        bool perfect_reflect=(bool)(bsdf->bsdf_type_&BSDFType::PerfectReflection);
        bool need_mis=needMIS(bsdf->bsdf_type_);

        if((int)(inst->material_->type_&MtlType::Emissive)  // if meet an Emitter
            &&glm::dot(curRay.dir_,inst->normal_)<0.f       // front face
            &&need_mis)                                     // need mis
        {
            auto Li=inst->material_->radiance_rgb_;
            float light_prob=scene.getLightPDF(curRay,*inst);

            float weight= perfect_reflect?1.0: 
                                         getMISweight(bsdfRec.pdf,light_prob);

            radiance+=throughput*Li*weight;
            break;
        }

        //-----------------------------------------------------------//
        /*--------------------- 2.INDIRECT LIGHT --------------------*/
        //-----------------------------------------------------------//

        if(max_depth_<=0){
            /* Russian Roulette */
            float RR=std::max(std::min((throughput[0]+throughput[1]+throughput[2])*0.3333333f,0.95f),0.2f);
            if(pRecord.sampler.pcgRNG_.nextFloat()<RR){
                throughput/=RR;
            }
            else
                break;
        }

    }
    
    return radiance;
}
```


## 几何求交与加速结构
采用BVH双层加速结构管理场景文件。所谓双层加速的BVH结构，其中底层加速结构(BLAS)管理唯一的obj模型，同一BLAS可以实例化为不同的对象（Instance），而顶层加速结构（TLAS）管理所有实例对象的所有属性。该结构一方面实现同一obj模型的内存复用，另一方面避免BLAS的冗余构建，且便于实时交互时的Instance属性动态更新。
在BVH的构建方面，我采用SAH的启发式做法，核心思想是每次划分时都考虑所有可能的划分方式，启发式为：`左节点的AABB面积*左节点的三角面片数+右节点的AABB面积*右节点的三角面片数`。

采用SAH算法显著提高了BVH的构建效率，下图对比了SAH和普通做法（排序+取中点）的构建差异。

![img/sah.png](assets/doc/img/sah.png)

为了光线求交的方便，我将TLAS和BLAS都作为为AccelStruct类的派生类，该基类提供了光线与BVH的遍历功能。
```c++
class AccelStruct{
public:
    AccelStruct():tree_(std::make_unique<std::vector<BVHnode>>()){}
    virtual ~AccelStruct(){}

    virtual bool traceRayInDetail(const Ray& ray,IntersectRecord& inst)const=0;
    bool traceRayInAccel(const Ray& ray,int32_t node_idx,IntersectRecord& inst,bool is_tlas)const;

    std::unique_ptr<std::vector<BVHnode>> tree_;                  

};
```
在遍历过程中，一旦光线与一个AS Instance的世界包围体相交，**就把光线变换到模型空间，对应到该 BLAS 做求交**；
- 接下来继续对局部空间下的包围体、三角形做更细的求交检测；
- 相交成功后再把交点、法线信息变换回世界坐标系。
```c++
bool TLAS::traceRayInDetail(const Ray& ray,IntersectRecord& inst)const{
    // find asinstance
    auto& node=tree_->at(inst.bvhnode_idx_);
    auto& instance=*all_instances_.at(node.prmitive_start);

    // transform ray into model's space
    auto mat_inv=instance.inv_modle_;
    auto morigin=mat_inv*glm::vec4(ray.origin_,1.0);
    auto mdir=mat_inv*glm::vec4(ray.dir_,0.0);
    Ray mray(morigin,mdir);

    // dive into blas
    if(instance.blas_->traceRayInAccel(mray,0,inst,false)){
        // transform intersect record back to world space.
        inst.pos_=instance.modle_*glm::vec4(inst.pos_,1.0);
        inst.normal_=glm::normalize(glm::vec3(glm::transpose(mat_inv)*glm::vec4(inst.normal_,0.0)));
        inst.t_=glm::length(inst.pos_-ray.origin_);
        
        return true;
    }
    return false;
}
```

## BSDF

BSDF类定义了每种材质的反射属性，主要包括sampleBSDF和evalBSDF两种方法，前者实现根据BRDF进行重要采样得到一个反射光线，后者基于该反射光线wi和入射光线wo得到pdf和bsdf值。下面简要说明需要注意的细节：
- **切线空间说明**：wi和wo都定义在光线与物体交点的切线空间，该空间通过一个TBN矩阵（切线、副切线、法向矩阵）来描述，wi和wo以交点为原点，以法向量为z轴。采样得到的wi和wo需要通过左乘该TBN矩阵回到模型空间，反之，模型空间的ray也需要左乘该矩阵的逆进入切线空间，不过由于TBN矩阵的标准正交阵，所以其逆与转置相等。TBN的构造参考自文献：https://graphics.pixar.com/library/OrthonormalB/paper.pdf
- **混合材质的实现**：不同的BSDF模型分别作为BSDF类的派生类存在。对于混合的BSDF，比如漫反射和blinn-phong混合的glossy材质，通过一个BSDFlist类保存，该类同样属于BSDF的派生类，其特点在于通过`randomSelcetBSDF`方法根据不同bsdf的权重随机选择一个bsdf作为交点在的当前bsdf，每次光线求交后，首先会调用交点的`getBSDF`方法，通过材质生成其BSDFlist，并随机选择一个bsdf，之后的采样和评估都只基于本次所选的bsdf进行。
```c++
/**
 * @brief BSDF modelizes the sampling and evaluation of scattering models.
 * It exposes functions such as: evaluating(bsdf value) and 
 * sampling([0,1]^2->[theta,phi]) the model, and querying the pdf(w_i).
 * Note that Wi and Wo are both in tangent space local to the hit point 
 * and both point in the direciton of Normal
 */
class BSDF{
public:
    BSDF(BSDFType type):bsdf_type_(type){}
    virtual ~BSDF(){}

    /**
     * @brief Sample a wi in tangent space with the information stored in BSDFRecord,including wo,inst,sampler and such.
     *        Have to fill wi and costheta
     */
    virtual void sampleBSDF(BSDFRecord& bsdfRec)const=0;

    /**
     * @brief After the sampling of wi, this function takes charge of calculate bsdf_value and pdf.
     *        So remember to sample before calling this.
     */
    virtual void evalBSDF(BSDFRecord& rec)const=0;

    /**
     * @brief When choosing among different bsdfs, each BSDF are required to offer a weight, 
     *        for example, LambertianBSDF's weight is albedo(Kd)
     */
    virtual float getWeight()const=0;

    /**
     * @brief static function for sampling in hemishpere with cos weighted
     * 
     * @param uniform : samples in [0,1]^2
     */
    static void SpHSphereCosWeight(float& theta,float& phi,const glm::vec2& uniform);

    static glm::vec3 polar2Cartesian(float theta,float phi);

public:
    BSDFType bsdf_type_;
    float prob_;
    
};
```

## 采样
在蒙特卡洛算法中，对半球上单位立体角的积分被离散的处理为：按照某种pdf(w)随机采样一个出射方向，用该方向求得的值除以pdf作为原积分的无偏估计，通过N个这样的估计器（spp=N）可以以$sqrt(N)$的速率收敛。本项目主要通过低差异性序列、光源采样、重要性采样来优化采样效率。
- Sampler类中提供了生成分层采样样本的方法，之后每当需要样本时，通过`getSample1D`和`getSample2D`即可得到预先生成的样本。
- Emitter类管理了所有光源，将光源微分面积*光源辐照度作为每一点的采样权重。



## TODO
- 添加xml读取功能，完善用户场景数据的输入