#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SM_MAX_STATES        16
#define SM_MAX_NAME_LEN      32
#define SM_MAX_TRANSITIONS   32

typedef int32_t sm_state_t;
typedef int32_t sm_event_t;
typedef int32_t sm_error_t;

#define SM_STATE_IDLE        0
#define SM_STATE_INIT        1
#define SM_STATE_READY       2
#define SM_STATE_ERROR       3
#define SM_STATE_RECOVER     4
#define SM_STATE_CUSTOM_BASE 100

#define SM_EVENT_BASE        0
#define SM_EVENT_ENTRY       ((sm_event_t)-1)
#define SM_EVENT_EXIT        ((sm_event_t)-2)

#define SM_OK                0
#define SM_ERR_INVALID       (-1)
#define SM_ERR_TRANSITION    (-2)
#define SM_ERR_NOT_FOUND     (-3)
#define SM_ERR_HANDLER       (-4)

typedef struct sm_machine sm_machine_t;

typedef sm_error_t (*sm_handler_t)(sm_machine_t *sm, sm_event_t event, void *data);

typedef struct {
    sm_state_t from;
    sm_state_t to;
} sm_transition_t;

typedef struct {
    sm_state_t    id;
    char          name[SM_MAX_NAME_LEN];
    sm_handler_t  on_entry;
    sm_handler_t  on_exit;
    sm_handler_t  on_event;
    sm_transition_t transitions[SM_MAX_TRANSITIONS];
    size_t        num_transitions;
} sm_state_info_t;

struct sm_machine {
    sm_state_info_t states[SM_MAX_STATES];
    size_t          num_states;
    sm_state_t      current_state;
    bool            initialized;
    void           *user_data;
    sm_error_t      last_error;
};

void      sm_init(sm_machine_t *sm, void *user_data);
sm_error_t sm_register_state(sm_machine_t *sm, const sm_state_info_t *info);
sm_error_t sm_add_transition(sm_machine_t *sm, sm_state_t from, sm_state_t to);
sm_error_t sm_transition(sm_machine_t *sm, sm_state_t target);
sm_error_t sm_dispatch(sm_machine_t *sm, sm_event_t event, void *data);
sm_state_t sm_get_state(const sm_machine_t *sm);
const char *sm_get_state_name(const sm_machine_t *sm, sm_state_t state);
sm_error_t sm_get_last_error(const sm_machine_t *sm);
void      sm_print_state(const sm_machine_t *sm);

#endif
