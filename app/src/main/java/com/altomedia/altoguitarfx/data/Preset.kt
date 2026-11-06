package com.altomedia.altoguitarfx.data

import com.altomedia.altoguitarfx.effects.EffectType
import org.json.JSONArray
import org.json.JSONObject

/**
 * Satu preset (BAB 13): menyimpan nama, urutan efek, status aktif, dan nilai
 * parameter tiap efek. Dibuat/dimuat dari JSON agar bisa disimpan di
 * SharedPreferences tanpa ketergantungan tambahan.
 */
data class Preset(
    val name: String,
    val order: List<Int>,
    val enabled: Set<Int>,
    val params: Map<Int, List<Float>>,
    val builtIn: Boolean = false
) {

    fun toJson(): JSONObject {
        val json = JSONObject()
        json.put("name", name)
        json.put("builtIn", builtIn)
        json.put("order", JSONArray(order))
        json.put("enabled", JSONArray(enabled.sorted()))
        val paramsJson = JSONObject()
        params.forEach { (type, values) ->
            paramsJson.put(type.toString(), JSONArray(values))
        }
        json.put("params", paramsJson)
        return json
    }

    companion object {

        fun fromJson(json: JSONObject): Preset {
            val orderJson = json.optJSONArray("order") ?: JSONArray()
            val order = (0 until orderJson.length()).map { orderJson.getInt(it) }

            val enabledJson = json.optJSONArray("enabled") ?: JSONArray()
            val enabled = (0 until enabledJson.length()).map { enabledJson.getInt(it) }.toSet()

            val params = mutableMapOf<Int, List<Float>>()
            json.optJSONObject("params")?.let { paramsJson ->
                paramsJson.keys().forEach { key ->
                    val valuesJson = paramsJson.optJSONArray(key) ?: return@forEach
                    val type = key.toIntOrNull() ?: return@forEach
                    params[type] = (0 until valuesJson.length()).map {
                        valuesJson.getDouble(it).toFloat()
                    }
                }
            }

            return Preset(
                name = json.optString("name", "Preset"),
                order = order,
                enabled = enabled,
                params = params,
                builtIn = json.optBoolean("builtIn", false)
            )
        }

        /** Membangun preset dari seluruh jenis efek dengan nilai bawaan. */
        fun fromDefaults(name: String, builtIn: Boolean = false): Preset {
            val all = EffectType.values()
            return Preset(
                name = name,
                order = all.map { it.id },
                enabled = emptySet(),
                params = all.associate { type ->
                    type.id to type.params.map { it.default }
                },
                builtIn = builtIn
            )
        }
    }
}
