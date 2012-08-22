//
//  SDL_joystick_c.c
//  iAmiga
//
//  Created by Stuart Carnie on 6/5/11.
//  Copyright 2011 Manomio LLC. All rights reserved.
//
#include "SDL_config.h"
#include "SDL_sysjoystick.h"
#include "SDL_joystick_c.h"

Uint8 SDL_numjoysticks = 0;
SDL_Joystick **SDL_joysticks = NULL;
static SDL_Joystick *default_joystick = NULL;

int
SDL_JoystickInit(void)
{
    int arraylen;
    int status;
    
    SDL_numjoysticks = 0;
    status = SDL_SYS_JoystickInit();
    if (status >= 0) {
        arraylen = (status + 1) * sizeof(*SDL_joysticks);
        SDL_joysticks = (SDL_Joystick **) SDL_malloc(arraylen);
        if (SDL_joysticks == NULL) {
            SDL_numjoysticks = 0;
        } else {
            SDL_memset(SDL_joysticks, 0, arraylen);
            SDL_numjoysticks = status;
        }
        status = 0;
    }
    default_joystick = NULL;
    return (status);
}

/*
 * Count the number of joysticks attached to the system
 */
int
SDL_NumJoysticks(void)
{
    return SDL_numjoysticks;
}

/*
 * Get the implementation dependent name of a joystick
 */
const char *
SDL_JoystickName(int device_index)
{
    if ((device_index < 0) || (device_index >= SDL_numjoysticks)) {
        SDL_SetError("There are %d joysticks available", SDL_numjoysticks);
        return (NULL);
    }
    return (SDL_SYS_JoystickName(device_index));
}

/*
 * Open a joystick for use - the index passed as an argument refers to
 * the N'th joystick on the system.  This index is the value which will
 * identify this joystick in future joystick events.
 *
 * This function returns a joystick identifier, or NULL if an error occurred.
 */
SDL_Joystick *
SDL_JoystickOpen(int device_index)
{
    int i;
    SDL_Joystick *joystick;
    
    if ((device_index < 0) || (device_index >= SDL_numjoysticks)) {
        SDL_SetError("There are %d joysticks available", SDL_numjoysticks);
        return (NULL);
    }
    
    /* If the joystick is already open, return it */
    for (i = 0; SDL_joysticks[i]; ++i) {
        if (device_index == SDL_joysticks[i]->index) {
            joystick = SDL_joysticks[i];
            ++joystick->ref_count;
            return (joystick);
        }
    }
    
    /* Create and initialize the joystick */
    joystick = (SDL_Joystick *) SDL_malloc((sizeof *joystick));
    if (joystick == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }
    
    SDL_memset(joystick, 0, (sizeof *joystick));
    joystick->index = device_index;
    if (SDL_SYS_JoystickOpen(joystick) < 0) {
        SDL_free(joystick);
        return NULL;
    }
    
    if (joystick->naxes > 0) {
        joystick->axes = (Sint16 *) SDL_malloc(joystick->naxes * sizeof(Sint16));
    }
    if (joystick->nhats > 0) {
        joystick->hats = (Uint8 *) SDL_malloc(joystick->nhats * sizeof(Uint8));
    }
    if (joystick->nballs > 0) {
        joystick->balls = (struct balldelta *) SDL_malloc(joystick->nballs * sizeof(*joystick->balls));
    }
    if (joystick->nbuttons > 0) {
        joystick->buttons = (Uint8 *) SDL_malloc(joystick->nbuttons * sizeof(Uint8));
    }
    if (((joystick->naxes > 0) && !joystick->axes)
        || ((joystick->nhats > 0) && !joystick->hats)
        || ((joystick->nballs > 0) && !joystick->balls)
        || ((joystick->nbuttons > 0) && !joystick->buttons)) {
        SDL_OutOfMemory();
        SDL_JoystickClose(joystick);
        return NULL;
    }
    if (joystick->axes) {
        SDL_memset(joystick->axes, 0, joystick->naxes * sizeof(Sint16));
    }
    if (joystick->hats) {
        SDL_memset(joystick->hats, 0, joystick->nhats * sizeof(Uint8));
    }
    if (joystick->balls) {
        SDL_memset(joystick->balls, 0,
                   joystick->nballs * sizeof(*joystick->balls));
    }
    if (joystick->buttons) {
        SDL_memset(joystick->buttons, 0, joystick->nbuttons * sizeof(Uint8));
    }
    
    /* Add joystick to list */
    ++joystick->ref_count;
    #if !SDL_EVENTS_DISABLED
    SDL_Lock_EventThread();
    #endif
    for (i = 0; SDL_joysticks[i]; ++i)
    /* Skip to next joystick */ ;
    SDL_joysticks[i] = joystick;
    #if !SDL_EVENTS_DISABLED
    SDL_Unlock_EventThread();
    #endif
    
    return (joystick);
}

/*
 * Checks to make sure the joystick is valid.
 */
int
SDL_PrivateJoystickValid(SDL_Joystick ** joystick)
{
    int valid;
    
    if (*joystick == NULL) {
        *joystick = default_joystick;
    }
    if (*joystick == NULL) {
        SDL_SetError("Joystick hasn't been opened yet");
        valid = 0;
    } else {
        valid = 1;
    }
    return valid;
}

/*
 * Get the device index of an opened joystick.
 */
int
SDL_JoystickIndex(SDL_Joystick * joystick)
{
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return (-1);
    }
    return (joystick->index);
}

/*
 * Get the number of multi-dimensional axis controls on a joystick
 */
int
SDL_JoystickNumAxes(SDL_Joystick * joystick)
{
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return (-1);
    }
    return (joystick->naxes);
}

/*
 * Get the number of hats on a joystick
 */
int
SDL_JoystickNumHats(SDL_Joystick * joystick)
{
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return (-1);
    }
    return (joystick->nhats);
}

/*
 * Get the number of trackballs on a joystick
 */
int
SDL_JoystickNumBalls(SDL_Joystick * joystick)
{
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return (-1);
    }
    return (joystick->nballs);
}

/*
 * Get the number of buttons on a joystick
 */
int
SDL_JoystickNumButtons(SDL_Joystick * joystick)
{
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return (-1);
    }
    return (joystick->nbuttons);
}

/*
 * Get the current state of an axis control on a joystick
 */
Sint16
SDL_JoystickGetAxis(SDL_Joystick * joystick, int axis)
{
    Sint16 state;
    
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return (0);
    }
    if (axis < joystick->naxes) {
        state = joystick->axes[axis];
    } else {
        SDL_SetError("Joystick only has %d axes", joystick->naxes);
        state = 0;
    }
    return (state);
}

/*
 * Get the current state of a hat on a joystick
 */
Uint8
SDL_JoystickGetHat(SDL_Joystick * joystick, int hat)
{
    Uint8 state;
    
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return (0);
    }
    if (hat < joystick->nhats) {
        state = joystick->hats[hat];
    } else {
        SDL_SetError("Joystick only has %d hats", joystick->nhats);
        state = 0;
    }
    return (state);
}

/*
 * Get the current state of a button on a joystick
 */
Uint8
SDL_JoystickGetButton(SDL_Joystick * joystick, int button)
{
    Uint8 state;
    
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return (0);
    }
    if (button < joystick->nbuttons) {
        state = joystick->buttons[button];
    } else {
        SDL_SetError("Joystick only has %d buttons", joystick->nbuttons);
        state = 0;
    }
    return (state);
}

/*
 * Close a joystick previously opened with SDL_JoystickOpen()
 */
void
SDL_JoystickClose(SDL_Joystick * joystick)
{
    int i;
    
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return;
    }
    
    /* First decrement ref count */
    if (--joystick->ref_count > 0) {
        return;
    }
    
#if !SDL_EVENTS_DISABLED
    /* Lock the event queue - prevent joystick polling */
    SDL_Lock_EventThread();
#endif    
    if (joystick == default_joystick) {
        default_joystick = NULL;
    }
    SDL_SYS_JoystickClose(joystick);
    
    /* Remove joystick from list */
    for (i = 0; SDL_joysticks[i]; ++i) {
        if (joystick == SDL_joysticks[i]) {
            SDL_memmove(&SDL_joysticks[i], &SDL_joysticks[i + 1],
                        (SDL_numjoysticks - i) * sizeof(joystick));
            break;
        }
    }
#if !SDL_EVENTS_DISABLED    
    /* Let the event thread keep running */
    SDL_Unlock_EventThread();
#endif    
    /* Free the data associated with this joystick */
    if (joystick->axes) {
        SDL_free(joystick->axes);
    }
    if (joystick->hats) {
        SDL_free(joystick->hats);
    }
    if (joystick->balls) {
        SDL_free(joystick->balls);
    }
    if (joystick->buttons) {
        SDL_free(joystick->buttons);
    }
    SDL_free(joystick);
}

void SDL_JoystickQuit(void) {
#if !SDL_EVENTS_DISABLED
    /* Stop the event polling */
    SDL_Lock_EventThread();
#endif
    SDL_numjoysticks = 0;
#if !SDL_EVENTS_DISABLED
    SDL_Unlock_EventThread();
#endif
    
    /* Quit the joystick setup */
    SDL_SYS_JoystickQuit();
    if (SDL_joysticks) {
        SDL_free(SDL_joysticks);
        SDL_joysticks = NULL;
    }
}

/* These are global for SDL_sysjoystick.c and SDL_events.c */

int
SDL_PrivateJoystickAxis(SDL_Joystick * joystick, Uint8 axis, Sint16 value)
{
    int posted;
    
    /* Update internal joystick state */
    joystick->axes[axis] = value;
    
    /* Post the event, if desired */
    posted = 0;
#if !SDL_EVENTS_DISABLED
    if (SDL_GetEventState(SDL_JOYAXISMOTION) == SDL_ENABLE) {
        SDL_Event event;
        event.type = SDL_JOYAXISMOTION;
        event.jaxis.which = joystick->index;
        event.jaxis.axis = axis;
        event.jaxis.value = value;
        if ((SDL_EventOK == NULL)
            || (*SDL_EventOK) (SDL_EventOKParam, &event)) {
            posted = 1;
            SDL_PushEvent(&event);
        }
    }
#endif /* !SDL_EVENTS_DISABLED */
    return (posted);
}

int
SDL_PrivateJoystickHat(SDL_Joystick * joystick, Uint8 hat, Uint8 value)
{
    int posted;
    
    /* Update internal joystick state */
    joystick->hats[hat] = value;
    
    /* Post the event, if desired */
    posted = 0;
#if !SDL_EVENTS_DISABLED
    if (SDL_GetEventState(SDL_JOYHATMOTION) == SDL_ENABLE) {
        SDL_Event event;
        event.jhat.type = SDL_JOYHATMOTION;
        event.jhat.which = joystick->index;
        event.jhat.hat = hat;
        event.jhat.value = value;
        if ((SDL_EventOK == NULL)
            || (*SDL_EventOK) (SDL_EventOKParam, &event)) {
            posted = 1;
            SDL_PushEvent(&event);
        }
    }
#endif /* !SDL_EVENTS_DISABLED */
    return (posted);
}

int
SDL_PrivateJoystickBall(SDL_Joystick * joystick, Uint8 ball,
                        Sint16 xrel, Sint16 yrel)
{
    int posted;
    
    /* Update internal mouse state */
    joystick->balls[ball].dx += xrel;
    joystick->balls[ball].dy += yrel;
    
    /* Post the event, if desired */
    posted = 0;
#if !SDL_EVENTS_DISABLED
    if (SDL_GetEventState(SDL_JOYBALLMOTION) == SDL_ENABLE) {
        SDL_Event event;
        event.jball.type = SDL_JOYBALLMOTION;
        event.jball.which = joystick->index;
        event.jball.ball = ball;
        event.jball.xrel = xrel;
        event.jball.yrel = yrel;
        if ((SDL_EventOK == NULL)
            || (*SDL_EventOK) (SDL_EventOKParam, &event)) {
            posted = 1;
            SDL_PushEvent(&event);
        }
    }
#endif /* !SDL_EVENTS_DISABLED */
    return (posted);
}

int
SDL_PrivateJoystickButton(SDL_Joystick * joystick, Uint8 button, Uint8 state)
{
    int posted;
#if !SDL_EVENTS_DISABLED
    SDL_Event event;
    
    switch (state) {
        case SDL_PRESSED:
            event.type = SDL_JOYBUTTONDOWN;
            break;
        case SDL_RELEASED:
            event.type = SDL_JOYBUTTONUP;
            break;
        default:
            /* Invalid state -- bail */
            return (0);
    }
#endif /* !SDL_EVENTS_DISABLED */
    
    /* Update internal joystick state */
    joystick->buttons[button] = state;
    
    /* Post the event, if desired */
    posted = 0;
#if !SDL_EVENTS_DISABLED
    if (SDL_GetEventState(event.type) == SDL_ENABLE) {
        event.jbutton.which = joystick->index;
        event.jbutton.button = button;
        event.jbutton.state = state;
        if ((SDL_EventOK == NULL)
            || (*SDL_EventOK) (SDL_EventOKParam, &event)) {
            posted = 1;
            SDL_PushEvent(&event);
        }
    }
#endif /* !SDL_EVENTS_DISABLED */
    return (posted);
}

void
SDL_JoystickUpdate(void)
{
    int i;
    
    for (i = 0; SDL_joysticks[i]; ++i) {
        SDL_SYS_JoystickUpdate(SDL_joysticks[i]);
    }
}