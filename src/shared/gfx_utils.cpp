#include "gfx_utils.hpp"

namespace dr
{

Mat4<f32> make_scale(Vec3<f32> const& scale)
{
    Mat4<f32> m = Mat4<f32>::Identity();
    m.diagonal().head<3>() = scale;
    return m;
}

Mat4<f32> make_translate(Vec3<f32> const& translate)
{
    Mat4<f32> m = Mat4<f32>::Identity();
    m.col(3).head<3>() = translate;
    return m;
}

Mat4<f32> make_scale_translate(Vec3<f32> const& scale, Vec3<f32> const& translate)
{
    Mat4<f32> m{};
    m.diagonal().head<3>() = scale;
    m.col(3) = translate.homogeneous();
    return m;
}

Mat2<f32> make_rotate(f32 const angle)
{
    f32 const c = std::cos(angle);
    f32 const s = std::sin(angle);

    Mat2<f32> m;
    m.col(0) << c, s;
    m.col(1) << -s, c;
    return m;
}

Mat3<f32> make_orthogonal(Vec3<f32> const& unit_x, Vec3<f32> const& xy)
{
    Vec3<f32> const unit_z = unit_x.cross(xy).normalized();

    Mat3<f32> m;
    m.col(0) = unit_x;
    m.col(1) = unit_z.cross(unit_x);
    m.col(2) = unit_z;
    return m;
}

Mat4<f32> make_affine(Mat3<f32> const& linear)
{
    Mat4<f32> m = Mat4<f32>::Identity();
    m.topLeftCorner<3, 3>() = linear;
    return m;
}

Mat4<f32> make_affine(Mat3<f32> const& linear, Vec3<f32> const& translate)
{
    Mat4<f32> m{};
    m.topLeftCorner<3, 3>() = linear;
    m.rightCols<1>() = translate.homogeneous();
    return m;
}

Mat4<f32> make_look_at(Vec3<f32> const& eye, Vec3<f32> const& target, Vec3<f32> const& up)
{
    Vec3<f32> const z = (eye - target).normalized();
    Vec3<f32> const x = up.cross(z).normalized();
    Vec3<f32> const y = z.cross(x);

    Mat4<f32> m;
    m.col(0) << x.x(), y.x(), z.x(), 0.0f;
    m.col(1) << x.y(), y.y(), z.y(), 0.0f;
    m.col(2) << x.z(), y.z(), z.z(), 0.0f;
    m.col(3) << -eye.dot(x), -eye.dot(y), -eye.dot(z), 1.0f;
    return m;
}

template <>
Mat4<f32> make_perspective<NdcType_Default>(f32 const fov_y, f32 const aspect, f32 const near, f32 const far)
{
    // NOTE: This assumes a right-handed y-up view space i.e. [-near, -far] in view space maps to
    // [0, 1] in NDC space

    f32 const y = std::tan(fov_y * 0.5f);

    Mat4<f32> m;
    m.col(0) << 1.0f / (y * aspect), 0.0f, 0.0f, 0.0f;
    m.col(1) << 0.0f, 1.0f / y, 0.0f, 0.0f;
    m.col(2) << 0.0f, 0.0f, far / (near - far), -1.0f;
    m.col(3) << 0.0f, 0.0f, -far * near / (far - near), 0.0f;

    return m;
}

template <>
Mat4<f32> make_perspective<NdcType_OpenGl>(f32 const fov_y, f32 const aspect, f32 const near, f32 const far)
{
    // NOTE: This assumes a right-handed y-up view space i.e. [-near, -far] in view space maps to
    // [-1, 1] in NDC space

    f32 const y = std::tan(fov_y * 0.5f);

    Mat4<f32> m;
    m.col(0) << 1.0f / (y * aspect), 0.0f, 0.0f, 0.0f;
    m.col(1) << 0.0f, 1.0f / y, 0.0f, 0.0f;
    m.col(2) << 0.0f, 0.0f, -(far + near) / (far - near), -1.0f;
    m.col(3) << 0.0f, 0.0f, -2.0f * far * near / (far - near), 0.0f;

    return m;
}

template <>
Mat4<f32> make_perspective<NdcType_Vulkan>(f32 const fov_y, f32 const aspect, f32 const near, f32 const far)
{
    // NOTE: This assumes a right-handed y-up view space i.e. [-near, -far] in view space maps to
    // [0, 1] in NDC space

    f32 const y = std::tan(fov_y * 0.5f);

    Mat4<f32> m;
    m.col(0) << 1.0f / (y * aspect), 0.0f, 0.0f, 0.0f;
    m.col(1) << 0.0f, -1.0f / y, 0.0f, 0.0f;
    m.col(2) << 0.0f, 0.0f, far / (near - far), -1.0f;
    m.col(3) << 0.0f, 0.0f, -far * near / (far - near), 0.0f;

    return m;
}

template <>
Mat4<f32> make_orthographic<NdcType_Default>(
    f32 const left,
    f32 const right,
    f32 const bottom,
    f32 const top,
    f32 const near,
    f32 const far)
{
    // NOTE: This assumes a right-handed y-up view space i.e. [-near, -far] in view space maps to
    // [0, 1] in NDC space

    f32 const inv_w = 1.0f / (right - left);
    f32 const inv_h = 1.0f / (top - bottom);
    f32 const inv_d = 1.0f / (near - far);

    Mat4<f32> m;
    m.col(0) << 2.0f * inv_w, 0.0f, 0.0f, 0.0f;
    m.col(1) << 0.0f, 2.0f * inv_h, 0.0f, 0.0f;
    m.col(2) << 0.0f, 0.0f, inv_d, 0.0f;
    m.col(3) << -(right + left) * inv_w, -(top + bottom) * inv_h, near * inv_d, 1.0f;

    return m;
}

template <>
Mat4<f32> make_orthographic<NdcType_OpenGl>(
    f32 const left,
    f32 const right,
    f32 const bottom,
    f32 const top,
    f32 const near,
    f32 const far)
{
    // NOTE: This assumes a right-handed y-up view space i.e. [-near, -far] in view space maps to
    // [-1, 1] in NDC space

    f32 const inv_w = 1.0f / (right - left);
    f32 const inv_h = 1.0f / (top - bottom);
    f32 const inv_d = 1.0f / (near - far);

    Mat4<f32> m;
    m.col(0) << 2.0f * inv_w, 0.0f, 0.0f, 0.0f;
    m.col(1) << 0.0f, 2.0f * inv_h, 0.0f, 0.0f;
    m.col(2) << 0.0f, 0.0f, 2.0f * inv_d, 0.0f;
    m.col(3) << -(right + left) * inv_w, -(top + bottom) * inv_h, (far + near) * inv_d, 1.0f;

    return m;
}

template <>
Mat4<f32> make_orthographic<NdcType_Vulkan>(
    f32 const left,
    f32 const right,
    f32 const bottom,
    f32 const top,
    f32 const near,
    f32 const far)
{
    // NOTE: This assumes a right-handed y-up view space i.e. [-near, -far] in view space maps to
    // [0, 1] in NDC space

    f32 const inv_w = 1.0f / (right - left);
    f32 const inv_h = 1.0f / (top - bottom);
    f32 const inv_d = 1.0f / (near - far);

    Mat4<f32> m;
    m.col(0) << 2.0f * inv_w, 0.0f, 0.0f, 0.0f;
    m.col(1) << 0.0f, -2.0f * inv_h, 0.0f, 0.0f;
    m.col(2) << 0.0f, 0.0f, inv_d, 0.0f;
    m.col(3) << -(right + left) * inv_w, (top + bottom) * inv_h, near * inv_d, 1.0f;

    return m;
}

Vec4<f32> to_plane_eqn(Vec3<f32> const& point, Vec3<f32> const& normal)
{
    return {normal.x(), normal.y(), normal.z(), -(normal.dot(point))};
}

} // namespace dr