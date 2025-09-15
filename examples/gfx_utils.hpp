#pragma once

#include <dr/math_types.hpp>

namespace dr
{

Mat4<f32> make_scale(Vec3<f32> const& scale);

Mat4<f32> make_translate(Vec3<f32> const& translate);

Mat4<f32> make_scale_translate(Vec3<f32> const& scale, Vec3<f32> const& translate);

Mat2<f32> make_rotate(f32 angle);

Mat3<f32> make_orthogonal(Vec3<f32> const& unit_x, Vec3<f32> const& xy);

Mat4<f32> make_affine(Mat3<f32> const& linear);

Mat4<f32> make_affine(Mat3<f32> const& linear, Vec3<f32> const& translate);

/// Creates a matrix that maps points from world space to view space. By convention, the view space
/// is right-handed y-up meaning it points in the negative z direction.
Mat4<f32> make_look_at(Vec3<f32> const& eye, Vec3<f32> const& target, Vec3<f32> const& up);

enum NdcType : u8
{
    NdcType_Default = 0, // z in [0, 1], y up
    NdcType_OpenGl, // z in [-1, 1], y up
    NdcType_Vulkan // z in [0, 1], y down
};

/// Creates a perspective projection matrix which maps points from view space (Cartesian
/// coordinates) to clip space (homogenous coordinates). This assumes a right-handed view space that
/// looks in the negative z direction.
template <NdcType ndc = NdcType_Default>
Mat4<f32> make_perspective(f32 fov_y, f32 aspect, f32 near, f32 far);

/// Creates a projection matrix which maps points from view space (Cartesian coordinates) to clip
/// space (homogenous coordinates). This assumes a right-handed view space that looks in the
/// negative z direction.
template <NdcType ndc = NdcType_Default>
Mat4<f32> make_orthographic(
    f32 const left,
    f32 const right,
    f32 const bottom,
    f32 const top,
    f32 const near,
    f32 const far);

/// Creates a projection matrix which maps points from view space (Cartesian coordinates) to clip
/// space (homogenous coordinates). This assumes a right-handed view space that looks in the
/// negative z direction.
template <NdcType ndc = NdcType_Default>
Mat4<f32> make_orthographic(
    f32 const height,
    f32 const aspect,
    f32 const near,
    f32 const far)
{
    f32 const half_h = height * 0.5f;
    f32 const half_w = half_h * aspect;
    return make_orthographic<ndc>(-half_w, half_w, -half_h, half_h, near, far);
}

/// Converts a point-normal representation of a plane to the equation representation i.e.
/// ax + by + cz + d = 0
Vec4<f32> to_plane_eqn(Vec3<f32> const& point, Vec3<f32> const& normal);

} // namespace dr