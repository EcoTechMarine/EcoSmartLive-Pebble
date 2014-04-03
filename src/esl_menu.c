#include "pebble.h"

#define NUM_MENU_SECTIONS 2
#define NUM_RADION_ITEMS 7
#define NUM_VORTECH_ITEMS 1
  
enum {	
  STATUS_KEY = 0,
	MESSAGE_KEY = 1
};

static Window *window;

// This is a menu layer
// You have more control than with a simple menu layer
static MenuLayer *menu_layer;

// Write message to buffer & send
void send_command(char *msg){
	DictionaryIterator *iter;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sending Message: %s", msg); 
	
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_KEY, msg);

	dict_write_end(iter);
  app_message_outbox_send();
}

// A callback is used to specify the amount of sections of menu items
// With this, you can dynamically add and remove sections
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return NUM_RADION_ITEMS;

    case 1:
      return NUM_VORTECH_ITEMS;

    default:
      return 0;
  }
}

// A callback is used to specify the height of the section header
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  // This is a define provided in pebble.h that you may use for the default height
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw what each header is
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Determine which section we're working with
  switch (section_index) {
    case 0:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, "RADION");
      break;

    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "VORTECH");
      break;
  }
}

// This is the menu item draw callback where you specify what each item should look like
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  // Determine which section we're going to draw in
  switch (cell_index->section) {
    case 0:
      // Use the row to specify which item we'll draw
      switch (cell_index->row) {
        case 0:
          menu_cell_title_draw(ctx, cell_layer, "5K");
          break;
        case 1:
          menu_cell_title_draw(ctx, cell_layer, "7K");
          break;
        case 2:
          menu_cell_title_draw(ctx, cell_layer, "10K");
          break;
        case 3:
          menu_cell_title_draw(ctx, cell_layer, "12K");
          break;
        case 4:
          menu_cell_title_draw(ctx, cell_layer, "14K");
          break;
        case 5:
          menu_cell_title_draw(ctx, cell_layer, "18K");
          break;
        case 6:
          menu_cell_title_draw(ctx, cell_layer, "20K");
          break;
      }
      break;

    case 1:
      switch (cell_index->row) {
        case 0:
          menu_cell_basic_draw(ctx, cell_layer, "Feed Mode", "Enter feed mode", NULL);
          break;
      }
  }
}

// Here we capture when a user selects a menu item
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  char		tmp[16] = { 0 };
  
  switch (cell_index->section) {
    case 0:
      snprintf(tmp, 15, "#R:PR:%d", cell_index->row + 1);
      send_command(tmp);
      break;

    case 1:
      switch (cell_index->row) {
        case 0:
          // TODO - send feed mode
          snprintf(tmp, 15, "#V:FM");
          send_command(tmp);
          break;
      }
  }
}

/*** App Message Callbacks ***/
// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	
	tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
	}
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

// This initializes the menu upon window load
void window_load(Window *window) {
  // Now we prepare to initialize the menu layer
  // We need the bounds to specify the menu layer's viewport size
  // In this case, it'll be the same as the window's
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  menu_layer = menu_layer_create(bounds);

  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(menu_layer, window);

  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
  
  // Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(menu_layer);
}

int main(void) {
  window = window_create();

  // Setup the window handlers
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_stack_push(window, true /* Animated */);
  
  send_command("?DLIST");

  app_event_loop();
  
  app_message_deregister_callbacks();

  window_destroy(window);
}