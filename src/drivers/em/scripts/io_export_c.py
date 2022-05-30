bl_info = {
    "name": "Export C source code (.c)",
    "author": 'Valtteri "tsone" Heikkila, Marton Ekler (original)',
    "version": (1, 1),
    "blender": (2, 6, 3),
    "location": "File > Export > C source code (.c)",
    "description": "Export mesh as C source code (.c)",
    "warning": "",
    "category": "Import-Export"}


import os
from math import *
import bpy
from bpy.props import *
import mathutils
from mathutils import *
from struct import *


out = None


def fmt_floats(values):
    return ','.join(('%.6f' % (v)).rstrip('0').rstrip('.') for v in values)


def export_mesh(out, obj, opts):
    print('--- Mesh START ---')
    print(str(obj))

    # Get mesh
    mesh = obj.to_mesh(bpy.context.scene, True, 'PREVIEW')
    mesh.update(calc_tessface = True)
    name = 'mesh_' + bpy.path.clean_name(obj.name)

    #out.write('const char obj_name[] = "%s";\n' % (obj.name))

    # Get world transformation (MODEL)
    matrix = obj.matrix_world
    #mesh.transform(matrix)
    # Apply axis-transformation for OpenGL
    T = Matrix(([1,0,0,0],[0,0,-1,0],[0,1,0,0],[0,0,0,1]))
    Tinv = T.copy()
    Tinv.invert()
    matrix = T * matrix * Tinv
    worldMatrix = []
    values = []
    for erow in matrix:
        for e in erow:
            values.append(e)
    out.write('const GLfloat %s_matrix[] = {%s};\n' % (name, fmt_floats(values)))

    # Vertex positions
    num_verts = len(mesh.vertices)
    out.write('const GLint %s_vert_num = %d;\n' % (name, num_verts))
    values = []
    for v in mesh.vertices:
        values += [v.co.x, v.co.z, -v.co.y]
    out.write('const GLfloat %s_verts[] = {%s};\n' % (name, fmt_floats(values)))

    # Vertex normals
    if opts.export_norms:
        values = []
        if opts.encode_norms:
            for v in mesh.vertices:
                x, y, z = v.normal.x, v.normal.z, -v.normal.y
                r = sqrt(x*x + y*y + z*z)
                c = [acos(z / r) / pi, ((atan2(y, x) + 2*pi) % (2*pi)) / (2*pi)]
                values += [str(int(65535*a + 0.5)) for a in c]
            out.write('const GLushort %s_norms[] = {%s};\n' % (name, ','.join(values)))
        else:
            for v in mesh.vertices:
                values += [v.normal.x, v.normal.z, -v.normal.y]
            out.write('const GLfloat %s_norms[] = {%s};\n' % (name, fmt_floats(values)))

    # Helper getter for vertex uv/color layer data
    def getActiveLayerData(layers):
        for layer in layers:
            if layer.active_render:
                return layer.data
        return None

    # Vertex UVs
    if opts.export_uvs and len(mesh.uv_textures):
        face_data = getActiveLayerData(mesh.tessface_uv_textures)
        if face_data:
            vertex_data = [[] for _ in range(num_verts)]
            # Build per-vertex UVs from tessfaces
            i = 0
            for face in face_data:
                tessface = mesh.tessfaces[i]
                vertex_data[tessface.vertices[0]] = face.uv1
                vertex_data[tessface.vertices[1]] = face.uv2
                vertex_data[tessface.vertices[2]] = face.uv3
                i = i + 1
            # Build linear array from data and write it
            values = []
            for v in vertex_data:
                if len(v) >= 2:
                    values += [v[0], v[1]]
                else:
                    values += [0, 0]
            out.write('const GLfloat %s_uvs[] = {%s};\n' % (name, fmt_floats(values)))

    # Vertex colors
    if opts.export_vcols and len(mesh.tessface_vertex_colors):
        face_data = getActiveLayerData(mesh.tessface_vertex_colors)
        if face_data:
            vertex_data = [[] for _ in range(num_verts)]
            # Build per-vertex colors from tessfaces
            i = 0
            for face in face_data:
                tessface = mesh.tessfaces[i]
                vertex_data[tessface.vertices[0]] = face.color1
                vertex_data[tessface.vertices[1]] = face.color2
                vertex_data[tessface.vertices[2]] = face.color3
                i = i + 1
            # Build linear array from data and write it
            values = []
            for v in vertex_data:
                if len(v) >= 3:
                    values += [str(int(255.0*c + 0.5)) for c in v[:3]]
                else:
                    values += "0,0,0"
            out.write('const GLubyte %s_vcols[] = {%s};\n' % (name, ','.join(values)))

    # Face indices
    out.write('const GLint %s_face_num = %d;\n' % (name, len(mesh.tessfaces)))
    #faceIndex = []
    values = []
    for face in mesh.tessfaces:
        i = 0
        for vert in face.vertices:
            i=i+1
            if i==4:
                raise Exception("There are quads in the mesh, only triangles meshed are supported")
            values.append(str(vert))
            #faceIndex.append(str(vert))
    if num_verts <= 256:
        out.write('const GLubyte')
    elif num_verts <= 65536:
        out.write('const GLushort')
    else:
        out.write('const GLuint')
    out.write(' %s_faces[] = {%s};\n' % (name, ','.join(values)))

    # Clean up temporary mesh
    bpy.data.meshes.remove(mesh)

    print('--- Mesh END ---')


def export_scene(path, objects, opts):
    scene = bpy.context.scene

    num_meshes = 0
    for obj in objects:
        print("obj.type", obj.type)
        if obj.type=='MESH':
            num_meshes = num_meshes + 1

    if num_meshes == 0:
        return

    out = open(path, 'w')
    out.write('//\n// DO NOT EDIT! FILE GENERATED BY BLENDER C SOURCE CODE EXPORTER\n//\n')

    #out.write('const char scene_name[] = "%s";\n\n' % (scene.name))

    out.write('const GLint mesh_num = %d;\n' % (num_meshes))
    for obj in objects:
        if obj.type=='MESH':
            export_mesh(out, obj, opts)

    out.close()


class CExporter(bpy.types.Operator):

    '''Export selected meshes as C source code'''

    bl_idname = "export.c"
    bl_label = "Export C source code"
    bl_options = {'PRESET'}

    objects = [] # <- Objects to export

    filepath = StringProperty(subtype='FILE_PATH')
    filename_ext = ".c"
    filter_glob = StringProperty(default="*.c", options={'HIDDEN'})

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.

    export_norms = BoolProperty(
            name="Export vertex normals",
            description="Export vertex normals",
            default=True,
            )
    export_uvs = BoolProperty(
            name="Export UVs",
            description="Export UV texture coordinates (if present)",
            default=True,
            )
    export_vcols = BoolProperty(
            name="Export vertex colors",
            description="Export vertex colors (if present)",
            default=True,
            )
    encode_norms = BoolProperty(
            name="Encode normals",
            description="Encode normals as two spherical coordinate angles (2-byte integers), "\
                "reducing size by 66.6%. Only works for unit-length normals and adds some error to the normals. "\
                "Decode normals with: x=sin(a)*cos(b), y=sin(a)*sin(b), z=cos(a), where "\
                "a=PI*c0/(2^16-1), b=2*PI*c1/(2^16-1), and c0,c1 are the angles",
            default=True,
            )


    def execute(self, context):
        if len(self.objects) < 1:
            return {"FINISHED"}

        pathname = self.filepath
        if pathname.endswith(self.filename_ext):
                pathname = pathname[0:-len(self.filename_ext)]

        """
        for obj in self.objects:
                fixed_name = bpy.path.clean_name(obj.name)
                filename = pathname + '_' + fixed_name + self.filename_ext
                print("Exporting to: " + filename)
                export_scene(filename, [obj])
        """
        export_scene(pathname + self.filename_ext, self.objects, self)
        self.objects = []
        return {"FINISHED"}


    def invoke(self, context, event):
        # Get selected objects
        self.objects = bpy.context.selected_objects
        if len(self.objects) < 1:
            return {"FINISHED"}

        WindowManager = context.window_manager
        WindowManager.fileselect_add(self)
        return {"RUNNING_MODAL"}


def menu_func_export(self, context):
    default_path = os.path.splitext(bpy.data.filepath)[0] + ".c"
    self.layout.operator(CExporter.bl_idname, text="C source code (.c)").filepath = default_path


def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

