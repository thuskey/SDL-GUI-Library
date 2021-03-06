//
//  GUIApp.cpp
//  Deep
//
//  Created by Nathan Daly on 11/27/12.
//  Copyright (c) 2012 Lions Entertainment. All rights reserved.
//

#include "GUIApp.h"
#include "GUIView.h"
#include "GUIWindow.h"

#include "GUIUtility.h"

#include "Compatibility.h"

#include <iostream>
#include TR1_FUNCTIONAL_H
using std::cout; using std::endl;
using std::list;
using std::tr1::bind;
using namespace std::tr1::placeholders;

using namespace GUIExceptionHandling;


namespace GUI {

// SINGLETON MEMBERS

App* App::singleton_ptr = 0;
App::App_destroyer App::the_App_destroyer;

App* App::get(){
	
	if (!singleton_ptr) {
		singleton_ptr = new App;
    }
	return singleton_ptr;
}

App::App_destroyer::~App_destroyer(){
	delete App::singleton_ptr;
}

// Forward Declarations
void print_msg(const Error &e);
void unhandled_click(const Unhandled_Click &e);


// App Implementation:

void print_msg(const Error &e) {
    cout << e.msg << endl;
}
void unhandled_click(const Unhandled_Click&) {
    cout << "unhandled click" << endl;
}

struct App_Quitter {
    App_Quitter(bool &running_) : running(running_) {}
    void operator()(Quit) { running = false; }
    bool &running;
};


App::App()
:window(0), fps_cap(FPS_CAP_DEFAULT), cap_frame_rate(true), running(false)
{ }



DispPoint App::get_screen_size() { return window->get_dim(); }


void App::quit() {
    running = false;
}


void App::run(Window* window_) {
    
    window = window_;
    
    register_exception_handler<Error>(&print_msg);

    running = true;

    register_exception_handler<Quit>(App_Quitter(running));
    
    window->refresh();
    
    
    if (timer_commands.size())
        next_timer_cmd = timer_commands.begin();
    
    while(running) {
        SDL_Event event;
        
        try {
            FrameRateCapper capper(fps_cap);
            
            if (cap_frame_rate)
            capper.cap_frame_rate();

            
            while (SDL_PollEvent(&event) && running){

                switch (event.type) {
                        
                    case SDL_MOUSEBUTTONDOWN: 
                    case SDL_MOUSEBUTTONUP:
                    case SDL_MOUSEMOTION: {
                        
                        // Send mouse event to correct view.

                        DispPoint click_pos(event.button.x, event.button.y);
                        DispPoint rel_pos(event.motion.xrel, event.motion.yrel);
                        
                        list<Controller*> focus_copy(captured_focus.begin(), captured_focus.end());
                        
                        for (list<Controller*>::iterator it = focus_copy.begin();
                                            it != focus_copy.end(); ++it) {
                            
                            Controller *captured = *it;
                            
                            DispPoint new_pos(click_pos);

                            // If the Controller is a view, adjust pos for view.
                            if (View *view = dynamic_cast<View*>(captured)) {
                                new_pos.x -= view->get_abs_pos().x; 
                                new_pos.y -= view->get_abs_pos().y; 
                            }
                            
                            bool handled;
                            
                            if (event.button.button == SDL_BUTTON_X1) {
                                cout << "SIDEWAYS SCROLL!" << endl;
                            }
                            if (event.button.button == SDL_BUTTON_WHEELUP) {
                                if (event.button.type == SDL_MOUSEBUTTONDOWN) {
                                    handled = captured->handle_mouse_scroll_start(true);
                                }
                                else if (event.button.type == SDL_MOUSEBUTTONUP) {
                                    handled = captured->handle_mouse_scroll_stop(true);
                                }
                            }
                            else if (event.button.button == SDL_BUTTON_WHEELDOWN) {
                                if (event.button.type == SDL_MOUSEBUTTONDOWN) {
                                    handled = captured->handle_mouse_scroll_start(false);
                                }
                                else if (event.button.type == SDL_MOUSEBUTTONUP) {
                                    handled = captured->handle_mouse_scroll_stop(false);
                                }
                            }
                            else {
                                if (event.button.type == SDL_MOUSEBUTTONDOWN) {
                                    handled = captured->handle_mouse_down(new_pos);
                                }
                                else if (event.button.type == SDL_MOUSEBUTTONUP) {
                                    handled = captured->handle_mouse_up(new_pos);
                                }
                                else if (event.button.type == SDL_MOUSEMOTION) {
                                    handled = captured->handle_mouse_motion(new_pos, rel_pos);
                                }
                            }
//                            if (!handled) {
//                                //...
//                            }
                        }
                        
                        View* hovered_view =
                        window->get_main_view()->get_view_from_point(click_pos);
                        

                        if (hovered_view) {
                            DispPoint new_pos(click_pos);

                            new_pos.x -= hovered_view->get_abs_pos().x; 
                            new_pos.y -= hovered_view->get_abs_pos().y; 
                          
                            if (event.button.button == SDL_BUTTON_WHEELUP) {
                                if (event.button.type == SDL_MOUSEBUTTONDOWN) {
                                    hovered_view->mouse_scroll_start(true);
                                }
                                else if (event.button.type == SDL_MOUSEBUTTONUP) {
                                    hovered_view->mouse_scroll_stop(true);
                                }
                            }
                            else if (event.button.button == SDL_BUTTON_WHEELDOWN) {
                                if (event.button.type == SDL_MOUSEBUTTONDOWN) {
                                    hovered_view->mouse_scroll_start(false);
                                }
                                else if (event.button.type == SDL_MOUSEBUTTONUP) {
                                    hovered_view->mouse_scroll_stop(false);
                                }
                            }
                            else {
                                
                                if (event.button.type == SDL_MOUSEBUTTONDOWN) {
                                    hovered_view->mouse_down(new_pos);
                                }
                                else if (event.button.type == SDL_MOUSEBUTTONUP) {
                                    hovered_view->mouse_up(new_pos);
                                }  
                                else if (event.button.type == SDL_MOUSEMOTION) {
                                    hovered_view->mouse_motion(new_pos, rel_pos);
                                }
                            }
                        } 
                        
                        break;
                    }
                    case SDL_KEYDOWN: {
                        cout << "KEYDOWN" << endl;
                        
                        
                        for (view_list_t::iterator it = captured_focus.begin();
                             it != captured_focus.end(); ++it) {
                            
                            Controller *captured = *it;
                            
                            bool handled = captured->handle_key_down(event.key.keysym);
                            if (!handled) {}
                        }

                        break;
                    }
                    case SDL_KEYUP: {
                        
                        cout << "KEYUP" << endl;

                        // Quit Key
                        if (event.key.keysym.sym == SDLK_q){
                            
#ifdef _MSC_VER  // Windows
                            if (event.key.keysym.mod == KMOD_LCTRL
								|| event.key.keysym.mod == KMOD_RCTRL
								|| event.key.keysym.mod == KMOD_CTRL){
                                throw Quit();
							}
#else           // Mac
							if (event.key.keysym.mod == KMOD_LMETA
								|| event.key.keysym.mod == KMOD_RMETA
								|| event.key.keysym.mod == KMOD_META){
                                throw Quit();
							}
#endif
						}
                        
                        for (view_list_t::iterator it = captured_focus.begin();
                             it != captured_focus.end(); ++it) {
                            
                            Controller *captured = *it;
                        
                            bool handled = captured->handle_key_up(event.key.keysym);
                            if (!handled) {}
                    
                        }
                        break;
                    }
   
                    case SDL_QUIT:
						throw Quit();
						break;
                        
					default:
						break;
                        
                }
            }

            cycle_timer_commands();            
        }
        
        catch(...) {
            
            call_exception_handlers(handler_list.begin(), handler_list.end());
            
        }
       
        window->refresh();
        
    }
}

void App::cycle_timer_commands() {
    
    for (size_t i = 0; i < timer_commands.size(); i++) {
        
        GUITimer_command *cmd = *next_timer_cmd;
        
        if (++next_timer_cmd == timer_commands.end()) {
            next_timer_cmd = timer_commands.begin();
        }
        
        cmd->execute_command();
    }
    
}

void App::cancel_timer_op(GUITimer_command* op) {
    std::vector<GUITimer_command*>::iterator it
    = std::find(timer_commands.begin(), timer_commands.end(), op);
    if (it != timer_commands.end()) {
        delete *it;
        timer_commands.erase(it);
    }
    else {
        throw Error("command not found!");
    }
    
    next_timer_cmd = timer_commands.begin();
}

} // namespace GUI


