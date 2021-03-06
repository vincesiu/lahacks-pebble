#include <pebble.h>

Window *window;	
static TextLayer *s_output_layer;
char name_buffer[64];
	
// Key values for AppMessage Dictionary
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1,
  KEY_BUTTON_EVENT = 2,
  BUTTON_EVENT_UP = 3,
  BUTTON_EVENT_DOWN = 4,
  BUTTON_EVENT_SELECT = 5,
};


//SENDS AN INTEGER CODE TO THE ANDROID APP SIDE  
void send_int(uint8_t key, uint8_t cmd)
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
      
    Tuplet value = TupletInteger(key, cmd);
    dict_write_tuplet(iter, &value);   
    app_message_outbox_send();
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

// Loads the main window of the papp
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create output TextLayer HOME DEFAULT SCREEN WHEN APP STARTS UP
  s_output_layer = text_layer_create(GRect(5, 0, window_bounds.size.w - 5, window_bounds.size.h));
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(s_output_layer, "Want to check in on Facebook? Press Select Button!");
  text_layer_set_overflow_mode(s_output_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer);
}

//SINGLE SELECT CLICK: SENDS A CHECKIN REQUEST TO THE ANDROID APP
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_output_layer, "Checked in! Press Down Button to Reset"); 
  send_int(KEY_BUTTON_EVENT, BUTTON_EVENT_SELECT);
}

//SINGLE DOWN CLICK: RESETS TO MAIN TEXT SCREEN
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  main_window_load(window);
}


// register click provider 
void config_provider(Window *window) {
 // single click 
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

//processes the incoming touples
void process_tuple(Tuple *t)
{
  app_log(APP_LOG_LEVEL_DEBUG,
                __FILE__,
                __LINE__,
                "Processing Tuple");
  //Get key
  int key = t->key;
  //text_layer_set_text(s_output_layer,(char*)&keyChar);
  //Get string value, if present
  char string_value[32];
  strcpy(string_value, (char*)t->value->cstring);
  
//display the person's name who is nearby
  if(key==STATUS_KEY){
      snprintf(name_buffer, 50, "%s is nearby! Click Down Button to Reset", string_value);
      text_layer_set_text(s_output_layer, (char*) &name_buffer);
  }
}

//handles the recieved data
static void in_received_handler(DictionaryIterator *iter, void *context) 
{
  // APP Sends information to the watch, display who's in range
  
    (void) context;
     
    //Get data
    Tuple *t = dict_read_first(iter);
    if(t)//VIBRATE ON POSITIVE ACK
    {
        vibes_short_pulse();
    }
  
    while(t)//parses through the dictionary and processes each tuple DOESNT WORK
    {
      switch(t->key){
        case MESSAGE_KEY:
          process_tuple(t);
          break;
        case STATUS_KEY:
          break;
      }
        app_log(APP_LOG_LEVEL_DEBUG,
                __FILE__,
                __LINE__,
                "Tuple Key=%i",
                (int)t->key);
         
        //Get next
        t = dict_read_next(iter);
    }
  
}



void init(void) {
	window = window_create();
  
    window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  //Register AppMessage events
  app_message_open(512, 512);    //Large input and output buffer sizes

	window_stack_push(window, true);
  window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
  //window_set_click_config_provider(s_main_window, click_config_provider);


	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	//send_message();
}

void deinit(void) { //destroys the the window and deallocates memory
	app_message_deregister_callbacks();
	window_destroy(window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}