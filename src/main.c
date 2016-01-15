#include <pebble.h>

#define KEY_VIBRATE 0

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_background_layer;
static TextLayer *s_battery_layer;
static BitmapLayer *s_charge_layer;
static GBitmap *s_charge_bitmap;
static BitmapLayer *s_bluetooth_layer;
static GBitmap *s_bluetooth_bitmap;
static TextLayer *s_date_layer;

static BatteryChargeState s_last_charge_state;

bool vibrateEnabled = true;

// Vibe pattern: ON for 200ms, OFF for 100ms, ON for 400ms:
static const uint32_t segments[] = { 500, 200, 500 };
VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};

/** Misc **/

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char bufferTime[] = "00:00";
  static char bufferDate[200] = "";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(bufferTime, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(bufferTime, sizeof("00:00"), "%I:%M", tick_time);
  }
  
  PBL_IF_ROUND_ELSE(
    strftime(bufferDate, sizeof(bufferDate), "%a %d %b", tick_time)
    ,
    strftime(bufferDate, sizeof(bufferDate), "%a %d %b %Y", tick_time)
  );

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, bufferTime);
  text_layer_set_text(s_date_layer, bufferDate);
}

static void update_battery(BatteryChargeState charge_state) {
  s_last_charge_state = charge_state;
  
  // Battery percentage
  static char battery_text[] = "100%";
  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  text_layer_set_text(s_battery_layer, battery_text);
  
  // Battery charging status
  if (charge_state.is_plugged) {
    bitmap_layer_set_bitmap(s_charge_layer, s_charge_bitmap);
  } else {
    bitmap_layer_set_bitmap(s_charge_layer, NULL);
  }

}

static void update_bluetooth(bool connected) {
  if (!connected) {
    if (!s_last_charge_state.is_plugged) {
      if (vibrateEnabled) {
        vibes_enqueue_custom_pattern(pat);
      }
    }
    bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bitmap);
  } else {
    bitmap_layer_set_bitmap(s_bluetooth_layer, NULL);
  }
}

/** TickTimerService **/

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Update battery status every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    update_battery(battery_state_service_peek());
  }
}

/** AppMessage callbacks **/

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create background TextLayer
  s_background_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  text_layer_set_background_color(s_background_layer, GColorBlack);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_background_layer));
  
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 50, bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  

  // Create charge BitmapLayer
  s_charge_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(80, 85), PBL_IF_ROUND_ELSE(12, 2), 20, 20));
  s_charge_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_charge_layer));

  
  // Create battery TextLayer
  s_battery_layer = text_layer_create(GRect(90, PBL_IF_ROUND_ELSE(20, 0), 54, 20));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_text(s_battery_layer, "");
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  
  
  // Create bluetooth BitmapLayer
  s_bluetooth_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(40, 0), PBL_IF_ROUND_ELSE(22, 2), 20, 20));
  s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NO_BT);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bluetooth_layer));
  
  
  // Create date Layer
  s_date_layer = text_layer_create(GRect(0, 130, bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  
  // Make sure info are displayed from the start
  update_time();
  update_battery(battery_state_service_peek());
  update_bluetooth(bluetooth_connection_service_peek());
}

static void main_window_unload(Window *window) {
  // Destroy all ressources
  text_layer_destroy(s_background_layer);
  text_layer_destroy(s_time_layer);
  gbitmap_destroy(s_charge_bitmap);
  bitmap_layer_destroy(s_charge_layer);
  text_layer_destroy(s_battery_layer);
  gbitmap_destroy(s_bluetooth_bitmap);
  bitmap_layer_destroy(s_bluetooth_layer);    
  text_layer_destroy(s_date_layer);
}  

static void in_recv_handler(DictionaryIterator *iterator, void *context)
{
  //Get Tuple
  Tuple *t = dict_read_first(iterator);
  if(t)
  {
    switch(t->key)
    {
    case KEY_VIBRATE:
      //It's the KEY_VIBRATE key
      if(strcmp(t->value->cstring, "on") == 0) {
        vibrateEnabled = true;
 
        persist_write_bool(KEY_VIBRATE, true);
      } else if(strcmp(t->value->cstring, "off") == 0) {
        vibrateEnabled = false;
 
        persist_write_bool(KEY_VIBRATE, false);
      }
      break;
    }
  }
}

/** App lifecycle **/

static void init() {
  // Use system locale
  setlocale(LC_ALL, "");
  
  // Get saved vibrate settings
  vibrateEnabled = persist_read_bool(KEY_VIBRATE);
  
  // Register with services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(&update_battery);
  bluetooth_connection_service_subscribe(&update_bluetooth);
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Set handlers for App Message
  app_message_register_inbox_received((AppMessageInboxReceived) in_recv_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}