#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "CLTypes.hpp"
#include "Vector3.hpp"

class Camera {
 public:
  Camera(CLTypes::Vector3 pos, CLTypes::Vector3 lookAt, CLTypes::Vector3 up,
         cl_float verticalFOV, cl_float aspect,
         cl_bool usePinholeLens = CL_TRUE, cl_float aperture = 0.0f,
         cl_float focusDist = 0.0f);

  void RotateCamera(cl_float degrees, cl_float deltaTime,
                    const CLTypes::Vector3& rotationPoint);

  CLTypes::Camera Calculate() const;

 private:
  cl_bool usePinholeLens;
  CLTypes::Vector3 origin;
  CLTypes::Vector3 lookAt;
  CLTypes::Vector3 up;
  cl_float halfWidth;
  cl_float halfHeight;
  cl_float focusDist;
  cl_float lensRadius;
};

#endif
