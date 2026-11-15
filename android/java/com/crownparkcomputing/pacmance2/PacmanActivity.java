package com.crownparkcomputing.pacmance2;

import org.libsdl.app.SDLActivity;

/** SDL owns the surface, gamepad and audio device; libmain runs the port. */
public class PacmanActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }
}
