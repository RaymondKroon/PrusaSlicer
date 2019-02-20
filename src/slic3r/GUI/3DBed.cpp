#include "libslic3r/libslic3r.h"

#include "3DBed.hpp"

#include "libslic3r/Polygon.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/BoundingBox.hpp"

#include "GUI_App.hpp"
#include "PresetBundle.hpp"

#include <GL/glew.h>

#include <boost/algorithm/string/predicate.hpp>

static const float GROUND_Z = -0.02f;

namespace Slic3r {
namespace GUI {

#if ENABLE_UNIQUE_BED
bool GeometryBuffer::set_from_triangles(const Polygons& triangles, float z, bool generate_tex_coords)
{
#if ENABLE_PRINTBED_SHADER
    m_vertices.clear();
    unsigned int v_size = 3 * (unsigned int)triangles.size();

    if (v_size == 0)
        return false;

    m_vertices = std::vector<Vertex>(v_size, Vertex());

    float min_x = unscale<float>(triangles[0].points[0](0));
    float min_y = unscale<float>(triangles[0].points[0](1));
    float max_x = min_x;
    float max_y = min_y;

    unsigned int v_count = 0;
    for (const Polygon& t : triangles)
    {
        for (unsigned int i = 0; i < 3; ++i)
        {
            Vertex& v = m_vertices[v_count];

            const Point& p = t.points[i];
            float x = unscale<float>(p(0));
            float y = unscale<float>(p(1));

            v.position[0] = x;
            v.position[1] = y;
            v.position[2] = z;

            if (generate_tex_coords)
            {
                v.tex_coords[0] = x;
                v.tex_coords[1] = y;

                min_x = std::min(min_x, x);
                max_x = std::max(max_x, x);
                min_y = std::min(min_y, y);
                max_y = std::max(max_y, y);
            }

            ++v_count;
        }
    }

    if (generate_tex_coords)
    {
        float size_x = max_x - min_x;
        float size_y = max_y - min_y;

        if ((size_x != 0.0f) && (size_y != 0.0f))
        {
            float inv_size_x = 1.0f / size_x;
            float inv_size_y = -1.0f / size_y;
            for (Vertex& v : m_vertices)
            {
                v.tex_coords[0] = (v.tex_coords[0] - min_x) * inv_size_x;
                v.tex_coords[1] = (v.tex_coords[1] - min_y) * inv_size_y;
            }
        }
    }
#else
    m_vertices.clear();
    m_tex_coords.clear();

    unsigned int v_size = 9 * (unsigned int)triangles.size();
    unsigned int t_size = 6 * (unsigned int)triangles.size();
    if (v_size == 0)
        return false;

    m_vertices = std::vector<float>(v_size, 0.0f);
    if (generate_tex_coords)
        m_tex_coords = std::vector<float>(t_size, 0.0f);

    float min_x = unscale<float>(triangles[0].points[0](0));
    float min_y = unscale<float>(triangles[0].points[0](1));
    float max_x = min_x;
    float max_y = min_y;

    unsigned int v_coord = 0;
    unsigned int t_coord = 0;
    for (const Polygon& t : triangles)
    {
        for (unsigned int v = 0; v < 3; ++v)
        {
            const Point& p = t.points[v];
            float x = unscale<float>(p(0));
            float y = unscale<float>(p(1));

            m_vertices[v_coord++] = x;
            m_vertices[v_coord++] = y;
            m_vertices[v_coord++] = z;

            if (generate_tex_coords)
            {
                m_tex_coords[t_coord++] = x;
                m_tex_coords[t_coord++] = y;

                min_x = std::min(min_x, x);
                max_x = std::max(max_x, x);
                min_y = std::min(min_y, y);
                max_y = std::max(max_y, y);
            }
        }
    }

    if (generate_tex_coords)
    {
        float size_x = max_x - min_x;
        float size_y = max_y - min_y;

        if ((size_x != 0.0f) && (size_y != 0.0f))
        {
            float inv_size_x = 1.0f / size_x;
            float inv_size_y = -1.0f / size_y;
            for (unsigned int i = 0; i < m_tex_coords.size(); i += 2)
            {
                m_tex_coords[i] = (m_tex_coords[i] - min_x) * inv_size_x;
                m_tex_coords[i + 1] = (m_tex_coords[i + 1] - min_y) * inv_size_y;
            }
        }
    }
#endif // ENABLE_PRINTBED_SHADER

    return true;
}

bool GeometryBuffer::set_from_lines(const Lines& lines, float z)
{
#if ENABLE_PRINTBED_SHADER
    m_vertices.clear();

    unsigned int v_size = 2 * (unsigned int)lines.size();
    if (v_size == 0)
        return false;

    m_vertices = std::vector<Vertex>(v_size, Vertex());

    unsigned int v_count = 0;
    for (const Line& l : lines)
    {
        Vertex& v1 = m_vertices[v_count];
        v1.position[0] = unscale<float>(l.a(0));
        v1.position[1] = unscale<float>(l.a(1));
        v1.position[2] = z;
        ++v_count;

        Vertex& v2 = m_vertices[v_count];
        v2.position[0] = unscale<float>(l.b(0));
        v2.position[1] = unscale<float>(l.b(1));
        v2.position[2] = z;
        ++v_count;
    }
#else
    m_vertices.clear();
    m_tex_coords.clear();

    unsigned int size = 6 * (unsigned int)lines.size();
    if (size == 0)
        return false;

    m_vertices = std::vector<float>(size, 0.0f);

    unsigned int coord = 0;
    for (const Line& l : lines)
    {
        m_vertices[coord++] = unscale<float>(l.a(0));
        m_vertices[coord++] = unscale<float>(l.a(1));
        m_vertices[coord++] = z;
        m_vertices[coord++] = unscale<float>(l.b(0));
        m_vertices[coord++] = unscale<float>(l.b(1));
        m_vertices[coord++] = z;
    }
#endif // ENABLE_PRINTBED_SHADER

    return true;
}

#if ENABLE_PRINTBED_SHADER
const float* GeometryBuffer::get_vertices_data() const
{
    return (m_vertices.size() > 0) ? (const float*)m_vertices.data() : nullptr;
}
#endif // ENABLE_PRINTBED_SHADER

unsigned int GeometryBuffer::get_vertices_count() const
{
#if ENABLE_PRINTBED_SHADER
    return (unsigned int)m_vertices.size();
#else
    return (unsigned int)m_vertices.size() / 3;
#endif // ENABLE_PRINTBED_SHADER
}

const double Bed3D::Axes::Radius = 0.5;
const double Bed3D::Axes::ArrowBaseRadius = 2.5 * Bed3D::Axes::Radius;
const double Bed3D::Axes::ArrowLength = 5.0;

Bed3D::Axes::Axes()
: origin(Vec3d::Zero())
, length(Vec3d::Zero())
{
    m_quadric = ::gluNewQuadric();
    if (m_quadric != nullptr)
        ::gluQuadricDrawStyle(m_quadric, GLU_FILL);
}

Bed3D::Axes::~Axes()
{
    if (m_quadric != nullptr)
        ::gluDeleteQuadric(m_quadric);
}

void Bed3D::Axes::render() const
{
    if (m_quadric == nullptr)
        return;

    glsafe(::glEnable(GL_DEPTH_TEST));
    glsafe(::glEnable(GL_LIGHTING));

    // x axis
    glsafe(::glColor3f(1.0f, 0.0f, 0.0f));
    glsafe(::glPushMatrix());
    glsafe(::glTranslated(origin(0), origin(1), origin(2)));
    glsafe(::glRotated(90.0, 0.0, 1.0, 0.0));
    render_axis(length(0));
    glsafe(::glPopMatrix());

    // y axis
    glsafe(::glColor3f(0.0f, 1.0f, 0.0f));
    glsafe(::glPushMatrix());
    glsafe(::glTranslated(origin(0), origin(1), origin(2)));
    glsafe(::glRotated(-90.0, 1.0, 0.0, 0.0));
    render_axis(length(1));
    glsafe(::glPopMatrix());

    // z axis
    glsafe(::glColor3f(0.0f, 0.0f, 1.0f));
    glsafe(::glPushMatrix());
    glsafe(::glTranslated(origin(0), origin(1), origin(2)));
    render_axis(length(2));
    glsafe(::glPopMatrix());

    glsafe(::glDisable(GL_LIGHTING));
}

void Bed3D::Axes::render_axis(double length) const
{
    ::gluQuadricOrientation(m_quadric, GLU_OUTSIDE);
    ::gluCylinder(m_quadric, Radius, Radius, length, 32, 1);
    ::gluQuadricOrientation(m_quadric, GLU_INSIDE);
    ::gluDisk(m_quadric, 0.0, Radius, 32, 1);
    glsafe(::glTranslated(0.0, 0.0, length));
    ::gluQuadricOrientation(m_quadric, GLU_OUTSIDE);
    ::gluCylinder(m_quadric, ArrowBaseRadius, 0.0, ArrowLength, 32, 1);
    ::gluQuadricOrientation(m_quadric, GLU_INSIDE);
    ::gluDisk(m_quadric, 0.0, ArrowBaseRadius, 32, 1);
}

Bed3D::Bed3D()
    : m_type(Custom)
#if ENABLE_PRINTBED_SHADER
    , m_vbo_id(0)
#endif // ENABLE_PRINTBED_SHADER
    , m_scale_factor(1.0f)
{
}

#if ENABLE_PRINTBED_SHADER
Bed3D::~Bed3D()
{
    reset();
}
#endif // ENABLE_PRINTBED_SHADER

bool Bed3D::set_shape(const Pointfs& shape)
{
    EType new_type = detect_type(shape);
    if (m_shape == shape && m_type == new_type)
        // No change, no need to update the UI.
        return false;

    m_shape = shape;
    m_type = new_type;

    calc_bounding_box();

    ExPolygon poly;
    for (const Vec2d& p : m_shape)
    {
        poly.contour.append(Point(scale_(p(0)), scale_(p(1))));
    }

    calc_triangles(poly);

    const BoundingBox& bed_bbox = poly.contour.bounding_box();
    calc_gridlines(poly, bed_bbox);

    m_polygon = offset_ex(poly.contour, (float)bed_bbox.radius() * 1.7f, jtRound, scale_(0.5))[0].contour;

#if ENABLE_PRINTBED_SHADER
    reset();
#endif // ENABLE_PRINTBED_SHADER

    // Set the origin and size for painting of the coordinate system axes.
    m_axes.origin = Vec3d(0.0, 0.0, (double)GROUND_Z);
    m_axes.length = 0.1 * get_bounding_box().max_size() * Vec3d::Ones();

    // Let the calee to update the UI.
    return true;
}

bool Bed3D::contains(const Point& point) const
{
    return m_polygon.contains(point);
}

Point Bed3D::point_projection(const Point& point) const
{
    return m_polygon.point_projection(point);
}

#if ENABLE_PRINTBED_SHADER
void Bed3D::render(float theta, bool useVBOs, float scale_factor) const
{
    m_scale_factor = scale_factor;

    EType type = useVBOs ? m_type : Custom;
    switch (type)

    {
    case MK2:
    {
        render_prusa("mk2", theta > 90.0f);
        break;
    }
    case MK3:
    {
        render_prusa("mk3", theta > 90.0f);
        break;
    }
    case SL1:
    {
        render_prusa("sl1", theta > 90.0f);
        break;
    }
    default:
    case Custom:
    {
        render_custom();
        break;
    }
    }
}
#else
void Bed3D::render(float theta, bool useVBOs, float scale_factor) const
{
    m_scale_factor = scale_factor;

    switch (m_type)
    {
    case MK2:
    {
        render_prusa("mk2", theta, useVBOs);
        break;
    }
    case MK3:
    {
        render_prusa("mk3", theta, useVBOs);
        break;
    }
    case SL1:
    {
        render_prusa("sl1", theta, useVBOs);
        break;
    }
    default:
    case Custom:
    {
        render_custom();
        break;
    }
    }
}
#endif // ENABLE_PRINTBED_SHADER

void Bed3D::render_axes() const
{
    if (!m_shape.empty())
        m_axes.render();
}

void Bed3D::calc_bounding_box()
{
    m_bounding_box = BoundingBoxf3();
    for (const Vec2d& p : m_shape)
    {
        m_bounding_box.merge(Vec3d(p(0), p(1), 0.0));
    }
}

void Bed3D::calc_triangles(const ExPolygon& poly)
{
    Polygons triangles;
    poly.triangulate(&triangles);

    if (!m_triangles.set_from_triangles(triangles, GROUND_Z, m_type != Custom))
        printf("Unable to create bed triangles\n");
}

void Bed3D::calc_gridlines(const ExPolygon& poly, const BoundingBox& bed_bbox)
{
    Polylines axes_lines;
    for (coord_t x = bed_bbox.min(0); x <= bed_bbox.max(0); x += scale_(10.0))
    {
        Polyline line;
        line.append(Point(x, bed_bbox.min(1)));
        line.append(Point(x, bed_bbox.max(1)));
        axes_lines.push_back(line);
    }
    for (coord_t y = bed_bbox.min(1); y <= bed_bbox.max(1); y += scale_(10.0))
    {
        Polyline line;
        line.append(Point(bed_bbox.min(0), y));
        line.append(Point(bed_bbox.max(0), y));
        axes_lines.push_back(line);
    }

    // clip with a slightly grown expolygon because our lines lay on the contours and may get erroneously clipped
    Lines gridlines = to_lines(intersection_pl(axes_lines, offset(poly, (float)SCALED_EPSILON)));

    // append bed contours
    Lines contour_lines = to_lines(poly);
    std::copy(contour_lines.begin(), contour_lines.end(), std::back_inserter(gridlines));

    if (!m_gridlines.set_from_lines(gridlines, GROUND_Z))
        printf("Unable to create bed grid lines\n");
}

Bed3D::EType Bed3D::detect_type(const Pointfs& shape) const
{
    EType type = Custom;

    auto bundle = wxGetApp().preset_bundle;
    if (bundle != nullptr)
    {
        const Preset* curr = &bundle->printers.get_selected_preset();
        while (curr != nullptr)
        {
            if (curr->config.has("bed_shape"))
            {
                if ((curr->vendor != nullptr) && (curr->vendor->name == "Prusa Research") && (shape == dynamic_cast<const ConfigOptionPoints*>(curr->config.option("bed_shape"))->values))
                {
                    if (boost::contains(curr->name, "SL1"))
                    {
                        type = SL1;
                        break;
                    }
                    else if (boost::contains(curr->name, "MK3") || boost::contains(curr->name, "MK2.5"))
                    {
                        type = MK3;
                        break;
                    }
                    else if (boost::contains(curr->name, "MK2"))
                    {
                        type = MK2;
                        break;
                    }
                }
            }

            curr = bundle->printers.get_preset_parent(*curr);
        }
    }

    return type;
}

#if ENABLE_PRINTBED_SHADER
void Bed3D::render_prusa(const std::string &key, bool bottom) const
{
    std::string tex_path = resources_dir() + "/icons/bed/" + key;

    std::string model_path = resources_dir() + "/models/" + key;

    // use anisotropic filter if graphic card allows
    GLfloat max_anisotropy = 0.0f;
    if (glewIsSupported("GL_EXT_texture_filter_anisotropic"))
        ::glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);

#if ENABLE_DISTANCE_FIELD_SHADER
    std::string filename = tex_path + "_df.png";
#else
    // use higher resolution images if graphic card allows
    GLint max_tex_size;
    ::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);

    // clamp or the texture generation becomes too slow
    max_tex_size = std::min(max_tex_size, 8192);

    std::string filename = tex_path + ".svg";
#endif // ENABLE_DISTANCE_FIELD_SHADER

    if ((m_texture.get_id() == 0) || (m_texture.get_source() != filename))
    {
#if ENABLE_DISTANCE_FIELD_SHADER
        if (!m_texture.load_from_file(filename, true))
#else
        if (!m_texture.load_from_svg_file(filename, true, max_tex_size))
#endif // ENABLE_DISTANCE_FIELD_SHADER
        {
            render_custom();
            return;
        }

        if (max_anisotropy > 0.0f)
        {
            ::glBindTexture(GL_TEXTURE_2D, m_texture.get_id());
            ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);
            ::glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    if (!bottom)
    {
        filename = model_path + "_bed.stl";
        if ((m_model.get_filename() != filename) && m_model.init_from_file(filename, true)) {
            Vec3d offset = m_bounding_box.center() - Vec3d(0.0, 0.0, 0.5 * m_model.get_bounding_box().size()(2));
            if (key == "mk2")
                // hardcoded value to match the stl model
                offset += Vec3d(0.0, 7.5, -0.03);
            else if (key == "mk3")
                // hardcoded value to match the stl model
                offset += Vec3d(0.0, 5.5, 2.43);
            else if (key == "sl1")
                // hardcoded value to match the stl model
                offset += Vec3d(0.0, 0.0, -0.03);

            m_model.center_around(offset);
        }

        if (!m_model.get_filename().empty())
        {
            ::glEnable(GL_LIGHTING);
            m_model.render();
            ::glDisable(GL_LIGHTING);
        }
    }

    unsigned int triangles_vcount = m_triangles.get_vertices_count();
    if (triangles_vcount > 0)
    {
        if (m_vbo_id == 0)
        {
            ::glGenBuffers(1, &m_vbo_id);
            ::glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
            ::glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)m_triangles.get_vertices_data_size(), (const GLvoid*)m_triangles.get_vertices_data(), GL_STATIC_DRAW);
            ::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, m_triangles.get_vertex_data_size(), (GLvoid*)m_triangles.get_position_offset());
            ::glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, m_triangles.get_vertex_data_size(), (GLvoid*)m_triangles.get_tex_coords_offset());
            ::glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        ::glEnable(GL_DEPTH_TEST);
        ::glDepthMask(GL_FALSE);

        ::glEnable(GL_BLEND);
        ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        ::glEnable(GL_TEXTURE_2D);
        ::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        if (bottom)
            ::glFrontFace(GL_CW);

        render_prusa_shader(triangles_vcount, bottom);

        if (bottom)
            ::glFrontFace(GL_CCW);

        ::glDisable(GL_TEXTURE_2D);

        ::glDisable(GL_BLEND);
        ::glDepthMask(GL_TRUE);
    }
}
#else
void Bed3D::render_prusa(const std::string &key, float theta, bool useVBOs) const
{
    std::string tex_path = resources_dir() + "/icons/bed/" + key;

    // use higher resolution images if graphic card allows
    GLint max_tex_size;
    ::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);

    // temporary set to lowest resolution
    max_tex_size = 2048;

    if (max_tex_size >= 8192)
        tex_path += "_8192";
    else if (max_tex_size >= 4096)
        tex_path += "_4096";

    std::string model_path = resources_dir() + "/models/" + key;

    // use anisotropic filter if graphic card allows
    GLfloat max_anisotropy = 0.0f;
    if (glewIsSupported("GL_EXT_texture_filter_anisotropic"))
        ::glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);

    std::string filename = tex_path + "_top.png";
    if ((m_top_texture.get_id() == 0) || (m_top_texture.get_source() != filename))
    {
        if (!m_top_texture.load_from_file(filename, true))
        {
            render_custom();
            return;
        }

        if (max_anisotropy > 0.0f)
        {
            ::glBindTexture(GL_TEXTURE_2D, m_top_texture.get_id());
            ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);
            ::glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    filename = tex_path + "_bottom.png";
    if ((m_bottom_texture.get_id() == 0) || (m_bottom_texture.get_source() != filename))
    {
        if (!m_bottom_texture.load_from_file(filename, true))
        {
            render_custom();
            return;
        }

        if (max_anisotropy > 0.0f)
        {
            ::glBindTexture(GL_TEXTURE_2D, m_bottom_texture.get_id());
            ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);
            ::glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    if (theta <= 90.0f)
    {
        filename = model_path + "_bed.stl";
        if ((m_model.get_filename() != filename) && m_model.init_from_file(filename, useVBOs)) {
            Vec3d offset = m_bounding_box.center() - Vec3d(0.0, 0.0, 0.5 * m_model.get_bounding_box().size()(2));
            if (key == "mk2")
                // hardcoded value to match the stl model
                offset += Vec3d(0.0, 7.5, -0.03);
            else if (key == "mk3")
                // hardcoded value to match the stl model
                offset += Vec3d(0.0, 5.5, 2.43);
            else if (key == "sl1")
                // hardcoded value to match the stl model
                offset += Vec3d(0.0, 0.0, -0.03);

            m_model.center_around(offset);
        }

        if (!m_model.get_filename().empty())
        {
            ::glEnable(GL_LIGHTING);
            m_model.render();
            ::glDisable(GL_LIGHTING);
        }
    }

    unsigned int triangles_vcount = m_triangles.get_vertices_count();
    if (triangles_vcount > 0)
    {
        ::glEnable(GL_DEPTH_TEST);
        ::glDepthMask(GL_FALSE);

        ::glEnable(GL_BLEND);
        ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        ::glEnable(GL_TEXTURE_2D);
        ::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        ::glEnableClientState(GL_VERTEX_ARRAY);
        ::glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        if (theta > 90.0f)
            ::glFrontFace(GL_CW);

        ::glBindTexture(GL_TEXTURE_2D, (theta <= 90.0f) ? (GLuint)m_top_texture.get_id() : (GLuint)m_bottom_texture.get_id());
        ::glVertexPointer(3, GL_FLOAT, 0, (GLvoid*)m_triangles.get_vertices());
        ::glTexCoordPointer(2, GL_FLOAT, 0, (GLvoid*)m_triangles.get_tex_coords());
        ::glDrawArrays(GL_TRIANGLES, 0, (GLsizei)triangles_vcount);

        if (theta > 90.0f)
            ::glFrontFace(GL_CCW);

        ::glBindTexture(GL_TEXTURE_2D, 0);
        ::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        ::glDisableClientState(GL_VERTEX_ARRAY);

        ::glDisable(GL_TEXTURE_2D);

        ::glDisable(GL_BLEND);
        ::glDepthMask(GL_TRUE);
    }
}
#endif // ENABLE_PRINTBED_SHADER

#if ENABLE_PRINTBED_SHADER
void Bed3D::render_prusa_shader(unsigned int vertices_count, bool transparent) const
{
    if (m_shader.get_shader_program_id() == 0)
#if ENABLE_DISTANCE_FIELD_SHADER
        m_shader.init("distance_field.vs", "distance_field.fs");
#else
        m_shader.init("printbed.vs", "printbed.fs");
#endif // ENABLE_DISTANCE_FIELD_SHADER

    if (m_shader.is_initialized())
    {
        m_shader.start_using();
        m_shader.set_uniform("transparent_background", transparent);

        ::glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture.get_id());
        ::glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
        ::glEnableVertexAttribArray(0);
        ::glEnableVertexAttribArray(1);
        ::glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices_count);
        ::glDisableVertexAttribArray(1);
        ::glDisableVertexAttribArray(0);
        ::glBindBuffer(GL_ARRAY_BUFFER, 0);
        ::glBindTexture(GL_TEXTURE_2D, 0);

        m_shader.stop_using();
    }
}
#endif // ENABLE_PRINTBED_SHADER

void Bed3D::render_custom() const
{
#if ENABLE_PRINTBED_SHADER
    m_texture.reset();
#else
    m_top_texture.reset();
    m_bottom_texture.reset();
#endif // ENABLE_PRINTBED_SHADER

    unsigned int triangles_vcount = m_triangles.get_vertices_count();
    if (triangles_vcount > 0)
    {
        glsafe(::glEnable(GL_LIGHTING));
        glsafe(::glDisable(GL_DEPTH_TEST));

        glsafe(::glEnable(GL_BLEND));
        glsafe(::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        glsafe(::glEnableClientState(GL_VERTEX_ARRAY));

        glsafe(::glColor4f(0.35f, 0.35f, 0.35f, 0.4f));
        glsafe(::glNormal3d(0.0f, 0.0f, 1.0f));
#if ENABLE_PRINTBED_SHADER
        ::glVertexPointer(3, GL_FLOAT, m_triangles.get_vertex_data_size(), (GLvoid*)m_triangles.get_vertices_data());
#else
        ::glVertexPointer(3, GL_FLOAT, 0, (GLvoid*)m_triangles.get_vertices());
#endif // ENABLE_PRINTBED_SHADER
        glsafe(::glDrawArrays(GL_TRIANGLES, 0, (GLsizei)triangles_vcount));

        // draw grid
        unsigned int gridlines_vcount = m_gridlines.get_vertices_count();

        // we need depth test for grid, otherwise it would disappear when looking the object from below
        glsafe(::glEnable(GL_DEPTH_TEST));
        glsafe(::glLineWidth(3.0f * m_scale_factor));
        glsafe(::glColor4f(0.2f, 0.2f, 0.2f, 0.4f));
#if ENABLE_PRINTBED_SHADER
        ::glVertexPointer(3, GL_FLOAT, m_triangles.get_vertex_data_size(), (GLvoid*)m_gridlines.get_vertices_data());
#else
        ::glVertexPointer(3, GL_FLOAT, 0, (GLvoid*)m_gridlines.get_vertices());
#endif // ENABLE_PRINTBED_SHADER
        glsafe(::glDrawArrays(GL_LINES, 0, (GLsizei)gridlines_vcount));

        glsafe(::glDisableClientState(GL_VERTEX_ARRAY));

        glsafe(::glDisable(GL_BLEND));
        glsafe(::glDisable(GL_LIGHTING));
    }
}

#if ENABLE_PRINTBED_SHADER
void Bed3D::reset()
{
    if (m_vbo_id > 0)
    {
        ::glDeleteBuffers(1, &m_vbo_id);
        m_vbo_id = 0;
    }
}
#endif // ENABLE_PRINTBED_SHADER
#endif // ENABLE_UNIQUE_BED

} // GUI
} // Slic3r