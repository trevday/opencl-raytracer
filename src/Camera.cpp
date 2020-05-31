#include "Camera.hpp"

Camera::Camera(CLTypes::Vector3 pos, CLTypes::Vector3 lookAt,
               CLTypes::Vector3 up, cl_float verticalFOV, cl_float aspect,
               cl_bool usePinholeLens /* = CL_TRUE */,
               cl_float aperture /* = 0.0f */, cl_float focusDist /* = 0.0f */)
    : usePinholeLens(usePinholeLens),
      lookAt(lookAt),
      up(up),
      focusDist(focusDist) {
  cl_float theta = verticalFOV * M_PI / 180.0;  // verticalFOV is in degrees
  halfHeight = tan(theta / 2.0);
  halfWidth = aspect * halfHeight;
  origin = pos;
  lensRadius = aperture / 2.0;
}

void Camera::RotateCamera(cl_float degrees, cl_float deltaTime,
                          const CLTypes::Vector3& rotationPoint) {
  glm::vec3 o(0, 0, 0);
  glm::vec3 rp = rotationPoint.ToGLMVec3();

  glm::mat4 matrix = glm::translate(glm::mat4(1.0f), rp - o);
  matrix =
      glm::rotate(matrix, degrees * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
  matrix = glm::translate(matrix, o - rp);

  glm::vec4 temp(origin.ToGLMVec3(), 1.0f);
  glm::vec4 rotated = matrix * temp;

  origin = CLTypes::Vector3(rotated.x, rotated.y, rotated.z);
}

CLTypes::Camera Camera::Calculate() const {
  CLTypes::Vector3 w = (origin - lookAt).Normalize();
  CLTypes::Vector3 u = up.Cross(w).Normalize();
  CLTypes::Vector3 v = w.Cross(u);

  CLTypes::Vector3 lowerLeftCorner;
  CLTypes::Vector3 horizontal;
  CLTypes::Vector3 vertical;
  if (usePinholeLens) {
    lowerLeftCorner = origin - halfWidth * u - halfHeight * v - w;
    horizontal = 2.0f * halfWidth * u;
    vertical = 2.0f * halfHeight * v;
  } else {
    lowerLeftCorner = origin - (halfWidth * focusDist * u) -
                      (halfHeight * focusDist * v) - (w * focusDist);
    horizontal = 2.0f * halfWidth * focusDist * u;
    vertical = 2.0f * halfHeight * focusDist * v;
  }

  CLTypes::Camera temp;
  temp.origin = origin.GetCLVector();
  temp.lower_left_corner = lowerLeftCorner.GetCLVector();
  temp.horizontal = horizontal.GetCLVector();
  temp.vertical = vertical.GetCLVector();
  temp.u = u.GetCLVector();
  temp.v = v.GetCLVector();
  temp.w = w.GetCLVector();
  temp.lens_radius = lensRadius;
  return temp;
}
