/* mbed Microcontroller Library
 * Copyright (c) 2018-2018 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef USBPHYHW_H
#define USBPHYHW_H

#include "mbed.h"
#include "USBPhy.h"

extern "C" {
	#include "nrf_drv_usbd.h"
	#include "nrf_drv_power.h"
}

class USBPhyHw : public USBPhy {

public:

	// Keep track of setup transaction stages
	typedef enum transaction_state_t {
		SetupStage,
		DataStage,
		StatusStage
	} transaction_state_t;

public:
    USBPhyHw();
    virtual ~USBPhyHw();
    virtual void init(USBPhyEvents *events);
    virtual void deinit();
    virtual bool powered();
    virtual void connect();
    virtual void disconnect();
    virtual void configure();
    virtual void unconfigure();
    virtual void sof_enable();
    virtual void sof_disable();
    virtual void set_address(uint8_t address);
    virtual void remote_wakeup();
    virtual const usb_ep_table_t* endpoint_table();

    virtual uint32_t ep0_set_max_packet(uint32_t max_packet);
    virtual void ep0_setup_read_result(uint8_t *buffer, uint32_t size);
    virtual void ep0_read(uint8_t *data, uint32_t size);
    virtual uint32_t ep0_read_result();
    virtual void ep0_write(uint8_t *buffer, uint32_t size);
    virtual void ep0_stall();

    virtual bool endpoint_add(usb_ep_t endpoint, uint32_t max_packet, usb_ep_type_t type);
    virtual void endpoint_remove(usb_ep_t endpoint);
    virtual void endpoint_stall(usb_ep_t endpoint);
    virtual void endpoint_unstall(usb_ep_t endpoint);

    virtual bool endpoint_read(usb_ep_t endpoint, uint8_t *data, uint32_t size);
    virtual uint32_t endpoint_read_result(usb_ep_t endpoint);
    virtual bool endpoint_write(usb_ep_t endpoint, uint8_t *data, uint32_t size);
    virtual void endpoint_abort(usb_ep_t endpoint);

    virtual void process();

    static void _usb_event_handler(nrf_drv_usbd_evt_t const * const p_event);
    static void _usb_power_event_handler(nrf_drv_power_usb_evt_t event);

    bool setup_feeder(nrf_drv_usbd_ep_transfer_t * p_next,
    							void * p_context,
    							size_t ep_size);

private:
    USBPhyEvents *events;

    bool sof_enabled;
    bool connect_enabled;

    typedef enum usb_hw_event_type_t {
    	USB_HW_EVENT_NONE  = 0,
		USB_HW_EVENT_USBD  = 1,
		USB_HW_EVENT_POWER = 2
    } usb_hw_event_type_t;

    // Event type to process
    usb_hw_event_type_t usb_event_type;

    // USB event buffer
    nrf_drv_usbd_evt_t usb_event;

    // USB power event buffer
    nrf_drv_power_usb_evt_t usb_power_event;

    // Buffer to hold setup packet
    nrf_drv_usbd_setup_t setup_buf;

    // State of the setup transaction
    transaction_state_t setup_state;

    // Setup bytes remaining
    uint32_t setup_remaining;

    // EP0 IN feeder
    nrf_drv_usbd_handler_desc_t ep0_in_handler;

    // Nordic transfer structures for each in/out endpoint
    nrf_drv_usbd_transfer_t transfer_buf[18];

    // Returns the appropraite transfer structure buffer for the given endpoint
    nrf_drv_usbd_transfer_t* get_transfer_buffer(usb_ep_t endpoint);

    // Returns the corresponding enumeration given an mbed endpoint number
    static nrf_drv_usbd_ep_t get_nordic_endpoint(usb_ep_t endpoint);

    void _reset(void);

    static void enable_usb_interrupts(void);
    static void disable_usb_interrupts(void);

};

#endif
