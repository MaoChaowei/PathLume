#include"common/common_include.h"
#include"interface.h"
#include"camera.h"
#include"buffer.h"
#include"scene_loader.h"

/**
 * @brief PathTracer encapsulates the top interface.
 * 
 */
class PathTracer{
public:
    PathTracer()=delete;
    PathTracer( const Camera* cam,const Scene* sce,std::shared_ptr<ColorBuffer> col){
        camera_=cam;
        scene_=sce;
        colorbuffer_=col;
    }
    bool Begin(const RTracingSetting& setting){
        // preprocess: create film and tiles
        std::shared_ptr<Film> film=camera_->getNewFilm();
        film->initTiles(setting.tiles_num_,colorbuffer_);
        
        // rendering
        film->parallelTiles();
        
        // emit finish signal
        return true;
    }

private:
    const Camera* camera_;
    std::shared_ptr<ColorBuffer> colorbuffer_;
    const Scene* scene_;

};