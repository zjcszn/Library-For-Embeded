/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @brief Multi-State object is an input object with a present-value that
 * uses an integer data type with a sequence of 1 to N values.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_MULTI_STATE_INPUT_H
#define BACNET_BASIC_OBJECT_MULTI_STATE_INPUT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - multistate preset-value prior to write
 * @param  value - multistate preset-value of the write
 */
typedef void (*multistate_input_write_present_value_callback)(
    uint32_t object_instance, uint32_t old_value,
    uint32_t value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Multistate_Input_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool Multistate_Input_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Multistate_Input_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Multistate_Input_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Multistate_Input_Instance_To_Index(
        uint32_t instance);

    BACNET_STACK_EXPORT
    int Multistate_Input_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool Multistate_Input_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    /* optional API */
    BACNET_STACK_EXPORT
    bool Multistate_Input_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool Multistate_Input_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Multistate_Input_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    uint32_t Multistate_Input_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Input_Present_Value_Set(
        uint32_t object_instance,
        uint32_t value);
    BACNET_STACK_EXPORT
    void Multistate_Input_Write_Present_Value_Callback_Set(
        multistate_input_write_present_value_callback cb);

    BACNET_STACK_EXPORT
    bool Multistate_Input_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Multistate_Input_Change_Of_Value_Clear(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Multistate_Input_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    BACNET_STACK_EXPORT
    bool Multistate_Input_Out_Of_Service(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Multistate_Input_Out_Of_Service_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    char *Multistate_Input_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Multistate_Input_Description_Set(
        uint32_t object_instance,
        char *text_string);

    BACNET_STACK_EXPORT
    BACNET_RELIABILITY Multistate_Input_Reliability(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Input_Reliability_Set(
        uint32_t object_instance,
        BACNET_RELIABILITY value);

    BACNET_STACK_EXPORT
    bool Multistate_Input_State_Text_List_Set(
        uint32_t object_instance,
        const char *state_text_list);
    BACNET_STACK_EXPORT
    bool Multistate_Input_State_Text_Set(
        uint32_t object_instance,
        uint32_t state_index,
        char *new_name);
    BACNET_STACK_EXPORT
    bool Multistate_Input_Max_States_Set(
        uint32_t instance,
        uint32_t max_states_requested);
    BACNET_STACK_EXPORT
    uint32_t Multistate_Input_Max_States(
        uint32_t instance);
    BACNET_STACK_EXPORT
    char *Multistate_Input_State_Text(
        uint32_t object_instance,
        uint32_t state_index);

    BACNET_STACK_EXPORT
    uint32_t Multistate_Input_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Input_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Multistate_Input_Cleanup(
        void);

    BACNET_STACK_EXPORT
    void Multistate_Input_Init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif