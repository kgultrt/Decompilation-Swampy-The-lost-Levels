package com.decompilationpixel.WMW;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class FileUtils {

    public static boolean createFile(String filePath, boolean isDir) throws IOException {
        Path path = Paths.get(filePath);
        if (isDir) {
            Files.createDirectories(path);
        } else {
            Files.createFile(path);
        }
        return Files.exists(path);
    }

    public static boolean deleteFile(String filePath) throws IOException {
        Path path = Paths.get(filePath);
        return Files.deleteIfExists(path);
    }

    public static void moveFile(String sourcePath, String targetPath) throws IOException {
        Path source = Paths.get(sourcePath);
        Path target = Paths.get(targetPath);
        Files.move(source, target);
    }

    public static void copyFile(String sourcePath, String targetPath) throws IOException {
        Path source = Paths.get(sourcePath);
        Path target = Paths.get(targetPath);
        Files.copy(source, target);
    }

    public static void renameFile(String name, String newName) throws IOException {
        moveFile(name, newName);
    }
}