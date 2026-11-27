package cn.crossdesk.mobile;

import android.content.Context;
import android.content.SharedPreferences;
import org.json.JSONArray;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** feature/parity: Android local key mapping store and resolver. */
final class RemoteKeyMaps {
    private static final String PREFS = "remote_keymaps_v1";
    private static final String ACTIVE = "active";
    private static final String RULES = "rules";

    static final class Rule {
        final String id;
        final boolean enabled;
        final String sourceType;
        final int sourceCode;
        final String actionType;
        final int[] keys;

        Rule(String id, boolean enabled, String sourceType, int sourceCode,
             String actionType, int[] keys) {
            this.id = id;
            this.enabled = enabled;
            this.sourceType = sourceType;
            this.sourceCode = sourceCode;
            this.actionType = actionType;
            this.keys = keys.clone();
        }
    }

    private static SharedPreferences prefs(Context context) {
        return context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    static List<Rule> rules(Context context) {
        List<Rule> result = new ArrayList<>();
        try {
            JSONArray array = new JSONArray(prefs(context).getString(RULES, "[]"));
            for (int i = 0; i < array.length(); i++) {
                JSONObject item = array.optJSONObject(i);
                if (item == null) continue;
                String id = item.optString("id", "");
                boolean enabled = item.optBoolean("enabled", true);
                String source = item.optString("sourceType", "key");
                int code = item.optInt("sourceCode", 0);
                String action = item.optString("actionType", "key");
                JSONArray values = item.optJSONArray("keys");
                int[] keys = new int[values == null ? 0 : values.length()];
                for (int j = 0; j < keys.length; j++) keys[j] = values.optInt(j);
                if (id.isEmpty() || !enabled) continue;
                result.add(new Rule(id, enabled, source, code, action, keys));
            }
        } catch (Exception ignored) {
        }
        return result;
    }

    static void save(Context context, List<Rule> values) {
        JSONArray array = new JSONArray();
        for (Rule rule : values) {
            JSONArray keys = new JSONArray();
            for (int key : rule.keys) keys.put(key);
            array.put(new JSONObject()
                    .put("id", rule.id)
                    .put("enabled", rule.enabled)
                    .put("sourceType", rule.sourceType)
                    .put("sourceCode", rule.sourceCode)
                    .put("actionType", rule.actionType)
                    .put("keys", keys));
        }
        prefs(context).edit().putString(RULES, array.toString()).apply();
    }

    static String active(Context context) {
        return prefs(context).getString(ACTIVE, "");
    }

    static void active(Context context, String name) {
        prefs(context).edit().putString(ACTIVE, name).apply();
    }

    /** Returns the mapped Windows virtual-key codes, or the original code. */
    static int[] resolve(Context context, int windowsCode) {
        for (Rule rule : rules(context)) {
            if (!rule.sourceType.equals("key") || rule.sourceCode != windowsCode) continue;
            if (rule.actionType.equals("key") || rule.actionType.equals("key_chord") ||
                rule.actionType.equals("macro")) {
                if (rule.keys.length > 0) return rule.keys.clone();
            }
        }
        return new int[]{windowsCode};
    }

    static String describe(Rule rule) {
        return rule.id + " " + rule.sourceType + ":" + rule.sourceCode + " -> " +
               rule.actionType + ":" + Arrays.toString(rule.keys);
    }
}
