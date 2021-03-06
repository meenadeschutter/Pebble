#include <pebble.h>
#include "VoiceRec.h"
#include "sqlite3.h"


#define NUM_QUESTIONS 5

static Window *s_main_window;
static TextLayer *s_question_layer, *s_prompt_layer;

static DictationSession *s_dictation_session;
static char s_last_text[256];

static char s_questions[5][64] = {
  "Where are you right now?",
  "How are you feeling?",
  "Who are you with?",
  "What were you doing?",
  "Do you have anything you want to tell me?"
};
static char s_answers_locations[5][32] = {
  //Locations
  "Home",
  "Work",
  "Gym",
  "Out",
  "University"
};
static char s_answers_emotions[9][32] = {
  //Feelings
  "Sad",
  "Happy",
  "Stressed",
  "Angry",
  "Excited",
  "Mad",
  "Worried",
  "Insecure",
  "Don't know"
};
static char s_answers_people[4][32] = {
  //People
  "Family",
  "Friend",
  "Pet",
  "Alone"
};
static char s_answers_actions[7][32] = {
  //Actions
  "Eat",
  "Sleep",
  "Exercis",
  "Socializ",
  "Work",
  "Cry",
  "Hacking"
};
static char s_answers_other[6][32] = {
  //Other
  "Other",
  "None",
  "Not applicable",
  "I don't know",
  "Yes",
  "No",
};

static int s_current_question;
static bool s_speaking_enabled;

/********************************* Quiz Logic *********************************/

static void next_question_handler(void *context) {
  if(s_current_question == NUM_QUESTIONS) {
    // Apply is over
    window_set_background_color(s_main_window, GColorDukeBlue);
    text_layer_set_text(s_question_layer, "Entry Submitted :)");

  } else {
    // Next question
    text_layer_set_text(s_question_layer, s_questions[s_current_question]);
    text_layer_set_text(s_prompt_layer, "Press Select to say your answer!");
    window_set_background_color(s_main_window, GColorDarkGray);
    s_speaking_enabled = true;
  }
}

static void check_answer(char *answer) {
  bool correct;
  int i;
  if (s_current_question == 0) {
    for (i = 0; i < 5; i++){
      correct = strstr(answer, s_answers_locations[i]);
      if (correct == true) {
        i = 5;
      }
    }
  } else if (s_current_question == 1) {
    for (i = 0; i < 9; i++) {
      correct = strstr(answer, s_answers_emotions[i]);
      if (correct == true) {
        i = 9;
      }
    }
  } else if (s_current_question == 2) {
    for (i = 0; i < 4; i++) {
      correct = strstr(answer, s_answers_people [i]);
      if (correct == true) {
        i = 4;
      }
    }
  }else if (s_current_question == 3) {
    for (i = 0; i < 7; i++) {
      correct = strstr(answer, s_answers_actions [i]);
      if (correct == true) {
        i = 7;
      }
    }
  } else {
    for (i = 0; i < 6; i++) {
      correct = strstr(answer, s_answers_other[i]);
      if (correct == true) {
        i = 6;
        
      }
    }
  }

  correct ? vibes_double_pulse() : vibes_long_pulse();
  text_layer_set_text(s_question_layer, correct ? "Thank you!" : "Sorry, we do not know that word yet!");
  text_layer_set_text(s_prompt_layer, (s_current_question == NUM_QUESTIONS - 1)
                      ? "" : "Message sent! Onto the next question :)");
  window_set_background_color(s_main_window, correct ? GColorGreen : GColorRed);

  s_current_question++;
  app_timer_register(3000, next_question_handler, NULL);
}

/******************************* Dictation API ********************************/

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status,
                                       char *transcription, void *context) {
  if(status == DictationSessionStatusSuccess) {
    // Check this answer
    check_answer(transcription);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Transcription failed.\n\nError ID:\n%d", (int)status);
  }
}

/************************************ App *************************************/

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_speaking_enabled) {
    // Start voice dictation UI
    dictation_session_start(s_dictation_session);
    s_speaking_enabled = false;
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_question_layer = text_layer_create(GRect(5, 10, bounds.size.w - 10, bounds.size.h));
  text_layer_set_font(s_question_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_color(s_question_layer, GColorWhite);
  text_layer_set_text_alignment(s_question_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_question_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_question_layer));

  s_prompt_layer = text_layer_create(GRect(5, 120, bounds.size.w - 10, bounds.size.h));
  text_layer_set_text(s_prompt_layer, "Press Select to start recording.");
  text_layer_set_font(s_prompt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_color(s_prompt_layer, GColorWhite);
  text_layer_set_text_alignment(s_prompt_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_prompt_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_prompt_layer));

#if defined(PBL_ROUND)
  const uint8_t inset = 3;

  text_layer_enable_screen_text_flow_and_paging(s_question_layer, inset);
  text_layer_enable_screen_text_flow_and_paging(s_prompt_layer, inset);
#endif
}

static void window_unload(Window *window) {
  text_layer_destroy(s_prompt_layer);
  text_layer_destroy(s_question_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  // Create new dictation session
  s_dictation_session = dictation_session_create(sizeof(s_last_text),
                                                 dictation_session_callback, NULL);

  window_set_background_color(s_main_window, GColorDarkGray);
  s_current_question = 0;
  text_layer_set_text(s_question_layer, s_questions[s_current_question]);
  s_speaking_enabled = true;
}

static void deinit() {
  // Free the last session data
  dictation_session_destroy(s_dictation_session);

  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}