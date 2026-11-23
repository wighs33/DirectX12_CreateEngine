"""
사용자가 만든 아바타 Luna를 T자세의 움직이지 않는 모델로 내보낸다(뼈 애니메이션은 나중 단계에서 다룬다).

    blender --background "<문서 폴더>/Untitled_refined.blend" --python Tools/Blender/export_luna.py

블렌드 파일은 읽기만 한다(저장하지 않는다). 이름을 바꾸거나 자세를 굳히는 것은 이 실행 안에서만 일어난다.
결과는 Assets/Models/Luna의 Luna.gltf, Luna.bin과 그림이다. Luna의 그림과 모양은 저장소에 올리지 않는다(.gitignore).
"""

import os
import shutil

import bpy

PROJECT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FOLDER = os.path.join(PROJECT, "Assets", "Models", "Luna")

RIG = "Armature"
# 기본 차림: 몸, 머리카락과 머리 장식, 원래 의상(조끼와 반바지)
MESHES = ["Body", "Hair", "HairPin", "HairChain", "Jacket", "Shorts"]
# 사람이 알아보기 쉬운 재질 이름. Pandora Arena의 Unity 재질과 같아서 그림체 값을 같은 이름으로 나눈다.
MATERIAL_NAMES = {
    "MAT_Tripo_NoHair_Character_Body": "Luna_Body",
    "MAT_Tripo_Hair_Halo": "Luna_Hair",
    "e79be0b4_c946_4c4d_9c1c_ab84e4ca8a68.001": "Luna_Jacket",
    "df4bb28a_f1ac_4771_990e_9081015c43c2.002": "Luna_Shorts",
    "MAT_Ornament_Gold": "Luna_Ornament",
    "MAT_Ornament_GoldAntique": "Luna_OrnamentAntique",
    "MAT_Ornament_Pearl": "Luna_OrnamentPearl",
}
# 파일의 기본 자세는 팔을 내린 A자세라서 이 액션의 자세로 굳힌다.
T_POSE_ACTION = "0_T-Pose.001"


def pose_as_t(rig):
    action = bpy.data.actions[T_POSE_ACTION]
    if rig.animation_data is None:
        rig.animation_data_create()
    rig.animation_data.action = action
    rig.data.pose_position = "POSE"
    bpy.context.scene.frame_set(int(action.frame_range[0]))
    bpy.context.view_layer.update()
    arm = rig.pose.bones["CC_Base_L_Upperarm"]
    direction = (rig.matrix_world @ arm.tail) - (rig.matrix_world @ arm.head)
    # 위팔이 거의 수평이어야 T자세다.
    assert abs(direction.normalized().z) < 0.2, "T자세가 아니다: %s" % direction


def check_images(meshes):
    """옮길 그림이 실제로 있는지 먼저 확인한다. 장식은 꼭짓점 색이라 그림이 없다."""
    for obj in meshes:
        for material in obj.data.materials:
            for node in material.node_tree.nodes:
                if node.type == "TEX_IMAGE" and node.image is not None:
                    source = bpy.path.abspath(node.image.filepath)
                    assert os.path.exists(source), "그림 파일이 없다: %s" % source


def freeze(meshes):
    """지금 자세의 모양을 뼈 없는 메시로 굳힌다. 좌표는 월드 기준이라 크기와 자리가 그대로 남는다."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    frozen = []
    for obj in meshes:
        evaluated = obj.evaluated_get(depsgraph)
        mesh = bpy.data.meshes.new_from_object(evaluated, preserve_all_data_layers=True, depsgraph=depsgraph)
        mesh.transform(obj.matrix_world)
        copy = bpy.data.objects.new("Luna_" + obj.name, mesh)
        bpy.context.scene.collection.objects.link(copy)
        frozen.append(copy)
    return frozen


def export(objects):
    if os.path.isdir(FOLDER):
        shutil.rmtree(FOLDER)
    os.makedirs(FOLDER)
    for obj in bpy.context.view_layer.objects:
        obj.select_set(False)
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.gltf(
        filepath=os.path.join(FOLDER, "Luna.gltf"),
        export_format="GLTF_SEPARATE",
        use_selection=True,
        export_yup=True,
        export_apply=False,
        export_animations=False,
        export_skins=False,
        export_morph=False,
        export_tangents=False,
    )


rig = bpy.data.objects[RIG]
sources = [bpy.data.objects[name] for name in MESHES]
for source in [rig] + sources:
    source.hide_viewport = False
    source.hide_set(False)
pose_as_t(rig)
check_images(sources)
for old, new in MATERIAL_NAMES.items():
    bpy.data.materials[old].name = new
export(freeze(sources))
print("exported", FOLDER)
