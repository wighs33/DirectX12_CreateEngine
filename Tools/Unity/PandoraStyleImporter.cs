using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

namespace MiniPandoras.Editor
{
    /// <summary>
    /// Pandora Engine의 그림체 파일(PandoraStyle.json)을 이름이 같은 툰 재질에 넣는다.
    /// 엔진의 Style 패널에서 고르고 저장한 값을 Unity에 다시 적지 않으려는 것이다.
    /// </summary>
    public static class PandoraStyleImporter
    {
        private const string MaterialsFolder = "Assets/MiniPandoras/Art/Materials/";
        private const string ToonShader = "MiniPandoras/Toon";
        private const string LastPathKey = "PandoraStyleImporter.LastPath";
        private const int SupportedVersion = 2;

        // 8비트 색 한 칸의 절반보다 작은 차이는 같은 색으로 본다. 같은 값을 다시 가져와도 재질이 바뀌지 않게 하려는 것이다.
        private const float ColorTolerance = 0.5f / 255f;
        // 엔진은 소수점 아래 네 자리까지 적는다.
        private const float ValueTolerance = 0.00005f;

        [MenuItem("MiniPandoras/Pandora Engine 그림체 가져오기")]
        private static void ImportFromMenu()
        {
            string lastPath = EditorPrefs.GetString(LastPathKey, string.Empty);
            string folder = string.IsNullOrEmpty(lastPath) ? string.Empty : Path.GetDirectoryName(lastPath);
            string path = EditorUtility.OpenFilePanel("PandoraStyle.json 고르기", folder, "json");
            if (string.IsNullOrEmpty(path))
            {
                return;
            }

            EditorPrefs.SetString(LastPathKey, path);
            Import(path);
        }

        /// <summary>바꾼 재질의 수를 돌려준다. 이름이 같은 재질이 없는 항목은 건너뛴다.</summary>
        public static int Import(string path)
        {
            StyleFile file = JsonUtility.FromJson<StyleFile>(File.ReadAllText(path));
            if (file == null || file.version != SupportedVersion || file.materials == null)
            {
                throw new InvalidDataException($"지원하지 않는 그림체 파일입니다: {path}");
            }

            int changedCount = 0;
            var skipped = new List<string>();
            foreach (Style style in file.materials)
            {
                Material material = FindMaterial(style.name);
                if (material == null)
                {
                    skipped.Add(style.name);
                    continue;
                }

                if (Apply(style, material))
                {
                    changedCount++;
                }
            }

            string skippedText = skipped.Count > 0 ? $" (Unity에 없는 재질: {string.Join(", ", skipped)})" : string.Empty;
            Debug.Log($"[그림체] 재질 {changedCount}개를 바꿨습니다{skippedText}");
            return changedCount;
        }

        private static Material FindMaterial(string name)
        {
            var material = AssetDatabase.LoadAssetAtPath<Material>($"{MaterialsFolder}{name}.mat");
            return material != null && material.shader.name == ToonShader ? material : null;
        }

        private static bool Apply(Style style, Material material)
        {
            Undo.RecordObject(material, "Pandora Engine 그림체 가져오기");

            bool isChanged = false;
            isChanged |= SetColor(material, "_ShadeColor", style.shadeColor, 1f);
            isChanged |= SetValue(material, "_ShadeThreshold", style.shadeThreshold);
            isChanged |= SetValue(material, "_ShadeSoftness", style.shadeSoftness);
            isChanged |= SetValue(material, "_AmbientWeight", style.ambientWeight);
            // 툰 셰이더는 가장자리 빛의 세기를 색의 알파에 둔다.
            isChanged |= SetColor(material, "_RimColor", style.rimColor, style.rimStrength);
            isChanged |= SetValue(material, "_RimWidth", style.rimWidth);
            isChanged |= SetColor(material, "_OutlineColor", style.outlineColor, 1f);
            isChanged |= SetValue(material, "_OutlineWidth", style.outlineWidth);

            if (isChanged)
            {
                EditorUtility.SetDirty(material);
                AssetDatabase.SaveAssetIfDirty(material);
            }

            return isChanged;
        }

        private static bool SetValue(Material material, string property, float value)
        {
            float current = material.GetFloat(property);
            if (Mathf.Abs(current - value) <= ValueTolerance)
            {
                return false;
            }

            material.SetFloat(property, value);
            Debug.Log($"[그림체] {material.name} {property}: {current} → {value}");
            return true;
        }

        private static bool SetColor(Material material, string property, float[] rgb, float alpha)
        {
            var color = new Color(rgb[0], rgb[1], rgb[2], alpha);
            Color current = material.GetColor(property);
            bool isSame = Mathf.Abs(current.r - color.r) <= ColorTolerance
                && Mathf.Abs(current.g - color.g) <= ColorTolerance
                && Mathf.Abs(current.b - color.b) <= ColorTolerance
                && Mathf.Abs(current.a - color.a) <= ValueTolerance;
            if (isSame)
            {
                return false;
            }

            material.SetColor(property, color);
            Debug.Log($"[그림체] {material.name} {property}: {current} → {color}");
            return true;
        }

        // 필드 이름은 JSON의 이름과 같아야 JsonUtility가 읽는다.
        [Serializable]
        private sealed class StyleFile
        {
            public int version;
            public Style[] materials;
        }

        [Serializable]
        private sealed class Style
        {
            public string name;
            public float[] shadeColor;
            public float shadeThreshold;
            public float shadeSoftness;
            public float ambientWeight;
            public float[] rimColor;
            public float rimStrength;
            public float rimWidth;
            public float[] outlineColor;
            public float outlineWidth;
        }
    }
}
