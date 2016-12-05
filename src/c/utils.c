/** upTempo for Pebble

    Copyright (C) 2015  Varun Shrivastav

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
**/

#include <pebble.h>
#include "utils.h"

GFont forcedSquare;
bool phoneDataSharing;

/**
 * create a text layer with all values
**/
TextLayer* macro_text_layer_create(GRect frame, Layer *parent, GColor tcolor, GColor bcolor, GFont font, GTextAlignment text_alignment) {
  TextLayer* layer = text_layer_create(frame);
  text_layer_set_text_color(layer, tcolor);
  text_layer_set_background_color(layer, bcolor);
  text_layer_set_text_alignment(layer, text_alignment);
  text_layer_set_font(layer, font);
  layer_add_child(parent, text_layer_get_layer(layer));
  return layer;
}

/**
 * Create bitmap layer with all values
**/
BitmapLayer* macro_bitmap_layer_create(GBitmap *bitmap, GRect frame, Layer *parent, bool visible) {
  BitmapLayer* layer = bitmap_layer_create(frame);
  bitmap_layer_set_compositing_mode(layer, BMP_COMPOSITING_MODE);
  bitmap_layer_set_alignment(layer, GAlignCenter);
  layer_add_child(parent, bitmap_layer_get_layer(layer));
  if(bitmap != NULL) bitmap_layer_set_bitmap(layer, bitmap);
  layer_set_hidden(bitmap_layer_get_layer(layer), !visible);
  return layer;
}

/**
 * Send message to phone
**/
void appmsg_send_val(int key, int value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &value, sizeof(int), true);
  app_message_outbox_send();
}