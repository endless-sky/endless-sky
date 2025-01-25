package com.github.thewierdnut.endless_mobile;


import org.libsdl.app.SDLActivity;
import android.content.Intent;
import java.io.OutputStream;
import java.io.InputStream;
import java.io.BufferedInputStream;
import android.net.Uri;
import java.io.IOException;
import android.app.Activity;
import android.widget.Toast;
import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.File;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;
import android.provider.OpenableColumns;
import android.database.Cursor;

import android.util.Log;

/**
    SDL Activity.
*/
public class ESActivity extends SDLActivity
{
    static int SAVE_FILE = 1;
    static int GET_FILE = 2;
    static int UNZIP_PLUGIN = 3;

    protected String[] getLibraries()
    {
        return new String[] {
            // Everything is statically linked
            // "SDL2",
            // "SDL2_image",
            // "SDL2_mixer",
            // "SDL2_net",
            // "SDL2_ttf",
            "main"
        };
    }

    // Call to save a file using an intent
    protected void saveFile(String filename, byte[] content)
    {
        save_file_content = content;
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_TITLE, filename);
        startActivityForResult(intent, SAVE_FILE);
    }

    // Call to request a file using an intent
    protected byte[] getFile(String prompt, String mime_type)
    {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(mime_type);
        loaded_file_content = null;
        synchronized(load_file_lock)
        {
            load_file_lock[0] = 0;
            startActivityForResult(Intent.createChooser(intent, prompt), GET_FILE);
            //startActivityForResult(intent, GET_FILE);
            // wait until intent finishes
            try
            {
                while (0 == load_file_lock[0])
                {
                    load_file_lock.wait();
                }
            }
            catch(InterruptedException e)
            {
                return null;
            }
        }
        byte[] ret = loaded_file_content;
        loaded_file_content = null;
        return ret;
    }

    // Call to unzip a plugin
    protected boolean promptUserAndUnzipPlugin(String prompt, String path)
    {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/zip");
        synchronized(zip_file_lock)
        {
            unzip_path = path + "/";
            zip_file_lock[0] = 0;
            startActivityForResult(Intent.createChooser(intent, prompt), UNZIP_PLUGIN);
            //startActivityForResult(intent, UNZIP_PLUGIN);
            // wait until intent finishes
            try
            {
                while (0 == zip_file_lock[0])
                {
                    zip_file_lock.wait();
                }
            }
            catch(InterruptedException e)
            {
                return false;
            }
        }
        return zip_file_lock[0] == 1;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if(requestCode == SAVE_FILE && resultCode == Activity.RESULT_OK)
        {
            Uri uri = data.getData();
            try
            {
                OutputStream output = getContext().getContentResolver().openOutputStream(uri);

                output.write(save_file_content);
                output.flush();
                output.close();
                save_file_content = null;
            }
            catch(IOException e)
            {
                Toast.makeText(this, "Endless-mobile unable to save file to " + uri.getEncodedPath(), Toast.LENGTH_SHORT).show();
            }
        }
        else if(requestCode == GET_FILE)
        {
            if (resultCode == Activity.RESULT_OK)
            {
                Uri uri = data.getData();
                try
                {
                    InputStream input = getContext().getContentResolver().openInputStream(uri);
                    ByteArrayOutputStream bs = new ByteArrayOutputStream();
                    byte[] buffer = new byte[4096];
                    int n = 0;
                    while ((n = input.read(buffer)) != -1)
                    {
                        bs.write(buffer, 0, n);
                    }
                    loaded_file_content = bs.toByteArray();
                    input.close();
                }
                catch(IOException e)
                {
                    loaded_file_content = null;
                    Toast.makeText(this, "Endless-mobile unable to open " + uri.getEncodedPath(), Toast.LENGTH_SHORT).show();
                }
            }
            // unblock the getFile function
            synchronized(load_file_lock)
            {
                load_file_lock[0] = 1;
                load_file_lock.notify();
            }
        }
        else if(requestCode == UNZIP_PLUGIN)
        {
            int status = 0;
            if (resultCode == Activity.RESULT_OK)
            {
                Uri uri = data.getData();
                try
                {
                    // Retrieve the filename from the data uri
                    String zipfilename = null;
                    if (uri.getScheme().equals("content"))
                    {
                        Cursor cursor = getContext().getContentResolver().query(uri, null, null, null, null);
                        try
                        {
                            if (cursor != null && cursor.moveToFirst())
                            {
                                zipfilename = cursor.getString(cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME));
                            }
                        }
                        finally
                        {
                            cursor.close();
                        }
                    }
                    if (zipfilename == null)
                    {
                        zipfilename = uri.getPath();
                        int cut = zipfilename.lastIndexOf('/');
                        if (cut != -1)
                        {
                            zipfilename = zipfilename.substring(cut + 1);
                        }
                    }
                    if (zipfilename != null && zipfilename.endsWith(".zip"))
                    {
                        zipfilename = zipfilename.substring(0, zipfilename.length() - 4);
                    }
                    
                    String unzipped_path = unzip_path;
                    if (zipfilename != null)
                    {
                        unzipped_path = unzipped_path + "tmp_" + zipfilename + "/";
                    }

                    File base_path = new File(unzipped_path);
                    base_path.mkdirs();

                    InputStream is = getContext().getContentResolver().openInputStream(uri);
                    ZipInputStream zip = new ZipInputStream(new BufferedInputStream(is));
                    ZipEntry e;
                    Boolean is_nested = true;
                    while ((e = zip.getNextEntry()) != null)
                    {
                        String path = unzipped_path + e.getName();
                        Log.e("SDL-Debug", "Unzipping " + path);

                        if (e.getName().startsWith("data/") ||
                            e.getName().startsWith("images/") ||
                            e.getName().startsWith("sounds/"))
                        {
                            is_nested = false;
                        }
                        if (e.isDirectory())
                        {
                            File f = new File(path);
                            f.mkdirs();
                        }
                        else
                        {
                            File f = new File(path);
                            f = f.getParentFile();
                            f.mkdirs();
                            FileOutputStream os = new FileOutputStream(path);
                            byte[] buffer = new byte[8192];
                            int len;
                            while ((len = zip.read(buffer)) != -1)
                            {
                                os.write(buffer, 0, len);
                            }
                            os.close();
                        }
                        zip.closeEntry();
                    }

                    zip.close();

                    // post-unzip cleanup
                    if (zipfilename != null)
                    {
                        if (is_nested && zipfilename != null)
                        {
                            // We have detected the plugin folders nested within
                            // a subfolder. Move everything up one level.
                            File oldDir = new File(unzipped_path);
                            for (String child: oldDir.list())
                            {
                                File nested = new File(unzipped_path + "/" + child);
                                nested.renameTo(new File(unzip_path + "/" + child));
                            }
                            oldDir.delete();
                        }
                        else
                        {
                            // The unzipped path has the correct structure.
                            // Strip off the tmp_tag
                            File oldDir = new File(unzipped_path);
                            oldDir.renameTo(new File(unzip_path + zipfilename));
                        }
                    }
                    Log.e("SDL-Debug", "Unzip finished");

                    status = 1; // success
                }
                catch(IOException e)
                {
                    Log.e("SDL-Debug", "IOException", e);
                    status = 2; // failure
                    Toast.makeText(this, "Endless-mobile unable to open " + uri.getEncodedPath(), Toast.LENGTH_SHORT).show();
                }
            }
            else
            {
                status = 2; // failure
            }
            // unblock the getFile function
            synchronized(zip_file_lock)
            {
                zip_file_lock[0] = status;
                zip_file_lock.notify();
            }
        }
    }

    byte[] save_file_content;
    byte[] loaded_file_content;
    int[] load_file_lock = new int[1];
    int[] zip_file_lock = new int[1]; // 0 pending, 1 success, 2 failure
    String unzip_path;
}


