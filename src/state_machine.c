#include "state_machine.h"
#include <stdio.h>
#include <string.h>

static sm_state_info_t *sm_find_state(sm_machine_t *sm, sm_state_t id)
{
    for (size_t i = 0; i < sm->num_states; i++) {
        if (sm->states[i].id == id) {
            return &sm->states[i];
        }
    }
    return NULL;
}

static bool sm_is_transition_valid(sm_state_info_t *state, sm_state_t target)
{
    for (size_t i = 0; i < state->num_transitions; i++) {
        if (state->transitions[i].to == target) {
            return true;
        }
    }
    return false;
}

void sm_init(sm_machine_t *sm, void *user_data)
{
    if (!sm) return;
    memset(sm, 0, sizeof(sm_machine_t));
    sm->user_data = user_data;
    sm->initialized = true;
    sm->current_state = SM_STATE_IDLE;
    sm->last_error = SM_OK;
}

sm_error_t sm_register_state(sm_machine_t *sm, const sm_state_info_t *info)
{
    if (!sm || !sm->initialized || !info) {
        return SM_ERR_INVALID;
    }
    if (sm->num_states >= SM_MAX_STATES) {
        return SM_ERR_INVALID;
    }
    if (sm_find_state(sm, info->id)) {
        return SM_ERR_INVALID;
    }

    sm_state_info_t *entry = &sm->states[sm->num_states++];
    memcpy(entry, info, sizeof(sm_state_info_t));
    if (strlen(info->name) > 0) {
        strncpy(entry->name, info->name, SM_MAX_NAME_LEN - 1);
        entry->name[SM_MAX_NAME_LEN - 1] = '\0';
    } else {
        snprintf(entry->name, SM_MAX_NAME_LEN, "STATE_%d", info->id);
    }
    return SM_OK;
}

sm_error_t sm_add_transition(sm_machine_t *sm, sm_state_t from, sm_state_t to)
{
    if (!sm || !sm->initialized) {
        return SM_ERR_INVALID;
    }
    sm_state_info_t *state = sm_find_state(sm, from);
    if (!state) {
        return SM_ERR_NOT_FOUND;
    }
    if (!sm_find_state(sm, to)) {
        return SM_ERR_NOT_FOUND;
    }
    if (state->num_transitions >= SM_MAX_TRANSITIONS) {
        return SM_ERR_INVALID;
    }
    state->transitions[state->num_transitions].from = from;
    state->transitions[state->num_transitions].to = to;
    state->num_transitions++;
    return SM_OK;
}

sm_error_t sm_transition(sm_machine_t *sm, sm_state_t target)
{
    if (!sm || !sm->initialized) {
        return SM_ERR_INVALID;
    }
    sm_state_info_t *current = sm_find_state(sm, sm->current_state);
    sm_state_info_t *next = sm_find_state(sm, target);
    if (!current || !next) {
        sm->last_error = SM_ERR_NOT_FOUND;
        return SM_ERR_NOT_FOUND;
    }

    if (!sm_is_transition_valid(current, target)) {
        sm->last_error = SM_ERR_TRANSITION;
        return SM_ERR_TRANSITION;
    }

    if (current->on_exit) {
        sm_error_t err = current->on_exit(sm, SM_EVENT_EXIT, sm->user_data);
        if (err != SM_OK) {
            sm->last_error = err;
            return SM_ERR_HANDLER;
        }
    }

    sm->current_state = target;

    if (next->on_entry) {
        sm_error_t err = next->on_entry(sm, SM_EVENT_ENTRY, sm->user_data);
        if (err != SM_OK) {
            sm->last_error = err;
            return SM_ERR_HANDLER;
        }
    }

    sm->last_error = SM_OK;
    return SM_OK;
}

sm_error_t sm_dispatch(sm_machine_t *sm, sm_event_t event, void *data)
{
    if (!sm || !sm->initialized) {
        return SM_ERR_INVALID;
    }
    sm_state_info_t *current = sm_find_state(sm, sm->current_state);
    if (!current) {
        sm->last_error = SM_ERR_NOT_FOUND;
        return SM_ERR_NOT_FOUND;
    }
    if (!current->on_event) {
        sm->last_error = SM_OK;
        return SM_OK;
    }
    sm_error_t err = current->on_event(sm, event, data ? data : sm->user_data);
    sm->last_error = err;
    return err;
}

sm_state_t sm_get_state(const sm_machine_t *sm)
{
    if (!sm) return SM_STATE_IDLE;
    return sm->current_state;
}

const char *sm_get_state_name(const sm_machine_t *sm, sm_state_t state)
{
    if (!sm) return "NULL";
    for (size_t i = 0; i < sm->num_states; i++) {
        if (sm->states[i].id == state) {
            return sm->states[i].name;
        }
    }
    return "UNKNOWN";
}

sm_error_t sm_get_last_error(const sm_machine_t *sm)
{
    if (!sm) return SM_ERR_INVALID;
    return sm->last_error;
}

void sm_print_state(const sm_machine_t *sm)
{
    if (!sm) {
        printf("[SM] NULL state machine\n");
        return;
    }
    printf("[SM] current_state=%d (%s) last_error=%d\n",
           sm->current_state,
           sm_get_state_name(sm, sm->current_state),
           sm->last_error);
}
