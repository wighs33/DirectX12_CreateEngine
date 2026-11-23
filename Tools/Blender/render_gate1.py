"""
관문 1 비교용 그림. 엔진과 같은 카메라와 빛으로 Luna를 그린다(블렌드 파일은 읽기만 한다).

    blender --background "<문서 폴더>/Untitled_refined.blend" --python Tools/Blender/render_gate1.py -- <결과.png>

엔진은 그림자와 윤곽 처리가 없는 램버트 조명이라, 여기서도 재질을 디퓨즈로 바꾸고 그림자를 끈다.
엔진 좌표 (x, y, z)는 블렌더 좌표 (x, z, y)다.
"""

import math
import os
import sys

import bpy
from mathutils import Vector

RIG = "Armature"
MESHES = ["Body", "Hair", "HairPin", "HairChain", "Jacket", "Shorts"]
T_POSE_ACTION = "0_T-Pose.001"
# 엔진에서 Luna를 놓은 자리와 카메라, 빛 (D3D12Context의 값)
LUNA_POSITION = (0.0, -0.95, 0.0)
CAMERA_POSITION = (0.0, 0.0, -3.0)
LIGHT_DIRECTION = (0.35, -0.75, 0.55)
LIGHT_COLOR = (1.0, 0.95, 0.85)
LIGHT_STRENGTH = 0.85
AMBIENT = (0.18, 0.20, 0.24)
BACKGROUND = (0.0100, 0.0331, 0.1329)


def to_blender(value):
    return Vector((value[0], value[2], value[1]))


def make_diffuse(material):
    """기본 색만 남긴 디퓨즈 재질로 바꾼다."""
    tree = material.node_tree
    output = next(node for node in tree.nodes if node.type == "OUTPUT_MATERIAL")
    source = next((node for node in tree.nodes if node.type == "TEX_IMAGE"), None)
    if source is None:
        source = next(node for node in tree.nodes if node.type == "VERTEX_COLOR")
    diffuse = tree.nodes.new("ShaderNodeBsdfDiffuse")
    tree.links.new(source.outputs["Color"], diffuse.inputs["Color"])
    tree.links.new(diffuse.outputs["BSDF"], output.inputs["Surface"])
    material.use_backface_culling = True


def build(path):
    scene = bpy.context.scene
    rig = bpy.data.objects[RIG]
    meshes = [bpy.data.objects[name] for name in MESHES]
    for obj in bpy.data.objects:
        obj.hide_render = obj not in meshes
    for obj in [rig] + meshes:
        obj.hide_viewport = False
        obj.hide_set(False)
    if rig.animation_data is None:
        rig.animation_data_create()
    action = bpy.data.actions[T_POSE_ACTION]
    rig.animation_data.action = action
    rig.data.pose_position = "POSE"
    scene.frame_set(int(action.frame_range[0]))
    for material in {slot.material for obj in meshes for slot in obj.material_slots}:
        make_diffuse(material)

    camera = bpy.data.objects.new("Gate1Camera", bpy.data.cameras.new("Gate1Camera"))
    scene.collection.objects.link(camera)
    # Luna를 옮기는 대신 카메라를 반대로 옮긴다.
    camera.location = to_blender(CAMERA_POSITION) - to_blender(LUNA_POSITION)
    camera.rotation_euler = (math.radians(90.0), 0.0, 0.0)
    camera.data.sensor_fit = "VERTICAL"
    camera.data.angle = math.radians(45.0)
    camera.data.clip_start = 0.1
    camera.data.clip_end = 100.0
    scene.camera = camera

    for obj in bpy.data.objects:
        if obj.type == "LIGHT":
            obj.hide_render = True
    sun = bpy.data.objects.new("Gate1Sun", bpy.data.lights.new("Gate1Sun", "SUN"))
    scene.collection.objects.link(sun)
    sun.rotation_euler = to_blender(LIGHT_DIRECTION).to_track_quat("-Z", "Y").to_euler()
    # 디퓨즈의 반사는 색/파이라서, 빛의 세기에 파이를 곱하면 엔진의 식과 같아진다.
    sun.data.energy = math.pi * LIGHT_STRENGTH
    sun.data.color = LIGHT_COLOR
    sun.data.angle = 0.0
    sun.data.use_shadow = False

    world = bpy.data.worlds.new("Gate1World")
    world.use_nodes = True
    scene.world = world
    tree = world.node_tree
    tree.nodes.clear()
    ambient = tree.nodes.new("ShaderNodeBackground")
    ambient.inputs["Color"].default_value = (*AMBIENT, 1.0)
    backdrop = tree.nodes.new("ShaderNodeBackground")
    backdrop.inputs["Color"].default_value = (*BACKGROUND, 1.0)
    ray = tree.nodes.new("ShaderNodeLightPath")
    mix = tree.nodes.new("ShaderNodeMixShader")
    output = tree.nodes.new("ShaderNodeOutputWorld")
    tree.links.new(ray.outputs["Is Camera Ray"], mix.inputs[0])
    tree.links.new(ambient.outputs[0], mix.inputs[1])
    tree.links.new(backdrop.outputs[0], mix.inputs[2])
    tree.links.new(mix.outputs[0], output.inputs["Surface"])

    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    scene.view_settings.exposure = 0.0
    scene.view_settings.gamma = 1.0
    scene.use_nodes = False
    if hasattr(scene.eevee, "use_raytracing"):
        scene.eevee.use_raytracing = False
    if hasattr(scene.eevee, "use_fast_gi"):
        scene.eevee.use_fast_gi = False
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


build(os.path.abspath(sys.argv[sys.argv.index("--") + 1]))
