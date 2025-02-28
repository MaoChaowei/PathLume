/* `Camera` defines the virtual camera and the properties of the final image */
#pragma once
#include"common_include.h"
#include"pathtracer/film.h"

class Window;

enum class CameraMovement{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	REFRESH,
};

class Camera{
public:
    Camera();
	Camera(glm::vec3 pos,glm::vec3 lookat,glm::vec3 right,float fov=60,float ratio=16.0/9.0,int image_width=1000);

	void setFrustrum(float fov,float near,float far);
	void setViewport(uint32_t width,float ratio);
    void setCameraPos(glm::vec3 pos,glm::vec3 lookat,glm::vec3 right);
	std::shared_ptr<Film> getNewFilm() const;

    glm::mat4 getViewMatrix()const;
	glm::mat4 getPerspectiveMatrix()const;
	glm::mat4 getViewportMatrix()const;
	float getNear()const{return near_flat_z_;}
	float getFar()const{return far_flat_z_;}

    const glm::vec3 getPosition()const{return this->position_;}
    const glm::vec3 getFront()const{return this->front_;}
    const glm::vec3 getRight()const{return this->right_;}
    const glm::vec3 getUp()const{return this->up_;}
    const int getImageWidth()const{return this->image_width_;}
    const int getImageHeight()const{return this->image_height_;}
	const bool needUpdateView();

	// ui
	void setMovement(float spd,float sensi);
	void processKeyboard(CameraMovement type,float delta);
	void processMouseMovement(float xoffset, float yoffset);
	void updateCameraVectors();


private:
	// Camera coordinate system in world coordinate system
	glm::vec3 position_;
	glm::vec3 front_;	// real direction
	glm::vec3 right_;
	glm::vec3 up_;

	bool update_view_flag_=false;		// turn true when camera moves and turn false when checked by `needUpdateMatrices`

	float speed_=0.8f;
	float sensitivity_=0.1f;
	float yaw_=0.f;    // horizental
    float pitch_=0.f;  // vertical

	// define the image and near flat
	
	float fov_=60;	// vertical fov
	float aspect_ratio_=16.0/9.0;
	int image_width_,image_height_;
	float half_near_height_,half_near_width_;

	//for frustrum
	float near_flat_z_=1;
	float far_flat_z_=1000;

	friend Window;
};
