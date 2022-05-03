package com.github.thewierdnut.endless_mobile;


import org.libsdl.app.SDLActivity;

/**
    SDL Activity. 
    We shouldn't need any code here. 
*/
public class ESActivity extends SDLActivity {
    protected String[] getLibraries() {
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
}


