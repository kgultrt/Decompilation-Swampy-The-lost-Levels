package com.decompilationpixel.WMW;

import android.Manifest;
import android.app.Activity;

public class Permission {
    static String[] permission = { 
        Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE };
    public static boolean getPermission(Activity context) {
        context.requestPermissions(permission, 114);
        return true;
    }
}
