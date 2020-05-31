#ifndef VECTOR_3_HPP
#define VECTOR_3_HPP

#include <OpenCL/opencl.h>

#include <glm/ext.hpp>

namespace CLTypes {

class Vector3 {
 public:
  Vector3() {}
  Vector3(cl_float x, cl_float y, cl_float z) {
    data.s[0] = x;
    data.s[1] = y;
    data.s[2] = z;
  }

  inline cl_float x() const { return data.s[0]; }
  inline cl_float y() const { return data.s[1]; }
  inline cl_float z() const { return data.s[2]; }

  Vector3 operator-(const Vector3& v2) const {
    return Vector3(x() - v2.x(), y() - v2.y(), z() - v2.z());
  }
  Vector3 operator*(const Vector3& v2) const {
    return Vector3(x() * v2.x(), y() * v2.y(), z() * v2.z());
  }
  Vector3 operator*(cl_float f) const {
    return Vector3(x() * f, y() * f, z() * f);
  }
  Vector3 operator/(cl_float f) const {
    return Vector3(x() / f, y() / f, z() / f);
  }

  cl_float SquaredLength() const { return x() * x() + y() * y() + z() * z(); }
  inline cl_float Length() const { return sqrt(SquaredLength()); }
  inline Vector3 Normalize() const { return (*this) / Length(); }

  inline Vector3 Cross(const Vector3& v2) const {
    return Vector3((y() * v2.z()) - (z() * v2.y()),
                   (z() * v2.x()) - (x() * v2.z()),
                   (x() * v2.y()) - (y() * v2.x()));
  }

  inline glm::vec3 ToGLMVec3() const { return glm::vec3(x(), y(), z()); }

  inline cl_float3 GetCLVector() const { return data; }

 private:
  cl_float3 data;
};

inline Vector3 operator*(cl_float f, const Vector3& v2) { return v2 * f; }

}  // namespace CLTypes

#endif
