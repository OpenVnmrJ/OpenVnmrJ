/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
/*
 *    This lock.h file is auto generated via Tau's CVS make base on the
 *    LOCK FPGA verilog source
 *    It is placed in our SCCS for consistency with our software
 */

#ifndef LOCK_H
#define LOCK_H
#include "fpga.h"
/*
#ifndef get_field
  #define get_field(fpga,field) \
    ((*((volatile unsigned *)(fpga##_BASE + fpga##_##field##_addr))>> \
    fpga##_##field##_pos) & (((fpga##_##field##_width<32) ? (1<<fpga##_##field##_width):0)-1))
#endif
#ifndef set_field
  #define set_field(fpga,field,value) \
    ((*((volatile unsigned *)(fpga##_BASE + fpga##_##field##_addr))) = \
    (*((volatile unsigned *)(fpga##_BASE + fpga##_##field##_addr))) & \
    ~((((fpga##_##field##_width<32)?(1<<fpga##_##field##_width):0)-1)<<fpga##_##field##_pos) | \
    ((value&(((fpga##_##field##_width<32)?(1<<fpga##_##field##_width):0)-1))<<fpga##_##field##_pos))
#endif
*/
#define LOCK_CHECKSUM 2532
#define LOCK_ID 0x0
#define LOCK_LED 0x4
#define LOCK_BoardAddress 0x8
#define LOCK_InterfaceType 0xc
#define LOCK_Failure 0x10
#define LOCK_DMARequest 0x14
#define LOCK_SCLKClockControl 0x18
#define LOCK_SCLKClockStatus 0x1c
#define LOCK_J1COutput 0x20
#define LOCK_J1CEnable 0x24
#define LOCK_J1CInput 0x28
#define LOCK_J2COutput 0x2c
#define LOCK_J2CEnable 0x30
#define LOCK_J2CInput 0x34
#define LOCK_InterruptStatus 0x38
#define LOCK_InterruptEnable 0x3c
#define LOCK_InterruptClear 0x40
#define LOCK_TransmitPhase 0x44
#define LOCK_TransmitPower 0x48
#define LOCK_ReceiverGain 0x4c
#define LOCK_LockAttenuation 0x50
#define LOCK_LockHiLow 0x54
#define LOCK_Locked 0x58
#define LOCK_SeqGenEnable 0x5c
#define LOCK_TransmitterStart 0x60
#define LOCK_TransmitterStop 0x64
#define LOCK_ReceiverStart 0x68
#define LOCK_ReceiverStop 0x6c
#define LOCK_CTCPulsePeriod 0x70
#define LOCK_NumCTCPulses 0x74
#define LOCK_ChannelPhase 0x78
#define LOCK_ChannelPulseWidth 0x7c
#define LOCK_ADCSpare 0x80
#define LOCK_ADCBufferPointer 0x84
#define LOCK_ADC_Count0 0x88
#define LOCK_ADC_Count1 0x8c
#define LOCK_DDSReset 0x90
#define LOCK_DDSFrequencyUpdate 0x94
#define LOCK_DDSFrequencyUpdateEnable 0x98
#define LOCK_DDSFrequencyUpdateOutput 0x9c
#define LOCK_LogicAnalyzerConfig 0xa0
#define LOCK_LogicAnalyzerStatus 0xa4
#define LOCK_LogicAnalyzerChecksum 0xa8
#define LOCK_LogicAnalyzerAddress 0xac
#define LOCK_LogicAnalyzerSampleData 0xb0
#define LOCK_ADCBuffer0 0x8000
#define LOCK_ADCBuffer1 0x10000
#define LOCK_ID_addr 0x0
#define LOCK_ID_pos 0
#define LOCK_ID_width 4
#define LOCK_revision_addr 0x0
#define LOCK_revision_pos 4
#define LOCK_revision_width 4
#define LOCK_checksum_addr 0x0
#define LOCK_checksum_pos 8
#define LOCK_checksum_width 16
#define LOCK_led_addr 0x4
#define LOCK_led_pos 0
#define LOCK_led_width 6
#define LOCK_bd_addr_addr 0x8
#define LOCK_bd_addr_pos 0
#define LOCK_bd_addr_width 5
#define LOCK_type_addr_addr 0xc
#define LOCK_type_addr_pos 0
#define LOCK_type_addr_width 3
#define LOCK_sw_failure_addr 0x10
#define LOCK_sw_failure_pos 0
#define LOCK_sw_failure_width 1
#define LOCK_sw_warning_addr 0x10
#define LOCK_sw_warning_pos 1
#define LOCK_sw_warning_width 1
#define LOCK_dma_request_addr 0x14
#define LOCK_dma_request_pos 0
#define LOCK_dma_request_width 2
#define LOCK_sclk_rst_addr 0x18
#define LOCK_sclk_rst_pos 0
#define LOCK_sclk_rst_width 1
#define LOCK_sclk_psincdec_addr 0x18
#define LOCK_sclk_psincdec_pos 1
#define LOCK_sclk_psincdec_width 1
#define LOCK_sclk_psen_addr 0x18
#define LOCK_sclk_psen_pos 2
#define LOCK_sclk_psen_width 1
#define LOCK_sclk_psclk_addr 0x18
#define LOCK_sclk_psclk_pos 3
#define LOCK_sclk_psclk_width 1
#define LOCK_sclk_locked_addr 0x1c
#define LOCK_sclk_locked_pos 0
#define LOCK_sclk_locked_width 1
#define LOCK_sclk_psdone_addr 0x1c
#define LOCK_sclk_psdone_pos 1
#define LOCK_sclk_psdone_width 1
#define LOCK_sclk_status_addr 0x1c
#define LOCK_sclk_status_pos 2
#define LOCK_sclk_status_width 3
#define LOCK_J1_C_out_addr 0x20
#define LOCK_J1_C_out_pos 0
#define LOCK_J1_C_out_width 32
#define LOCK_J1_C_en_addr 0x24
#define LOCK_J1_C_en_pos 0
#define LOCK_J1_C_en_width 32
#define LOCK_J1_C_addr 0x28
#define LOCK_J1_C_pos 0
#define LOCK_J1_C_width 32
#define LOCK_J2_C_out_addr 0x2c
#define LOCK_J2_C_out_pos 0
#define LOCK_J2_C_out_width 32
#define LOCK_J2_C_en_addr 0x30
#define LOCK_J2_C_en_pos 0
#define LOCK_J2_C_en_width 32
#define LOCK_J2_C_addr 0x34
#define LOCK_J2_C_pos 0
#define LOCK_J2_C_width 32
#define LOCK_fail_int_status_addr 0x38
#define LOCK_fail_int_status_pos 0
#define LOCK_fail_int_status_width 1
#define LOCK_warn_int_status_addr 0x38
#define LOCK_warn_int_status_pos 1
#define LOCK_warn_int_status_width 1
#define LOCK_adc_overflow_int_status_addr 0x38
#define LOCK_adc_overflow_int_status_pos 2
#define LOCK_adc_overflow_int_status_width 1
#define LOCK_adc_buffer_complete_int_0_status_addr 0x38
#define LOCK_adc_buffer_complete_int_0_status_pos 3
#define LOCK_adc_buffer_complete_int_0_status_width 1
#define LOCK_adc_buffer_complete_int_1_status_addr 0x38
#define LOCK_adc_buffer_complete_int_1_status_pos 4
#define LOCK_adc_buffer_complete_int_1_status_width 1
#define LOCK_fail_int_enable_addr 0x3c
#define LOCK_fail_int_enable_pos 0
#define LOCK_fail_int_enable_width 1
#define LOCK_warn_int_enable_addr 0x3c
#define LOCK_warn_int_enable_pos 1
#define LOCK_warn_int_enable_width 1
#define LOCK_adc_overflow_int_enable_addr 0x3c
#define LOCK_adc_overflow_int_enable_pos 2
#define LOCK_adc_overflow_int_enable_width 1
#define LOCK_adc_buffer_complete_int_0_enable_addr 0x3c
#define LOCK_adc_buffer_complete_int_0_enable_pos 3
#define LOCK_adc_buffer_complete_int_0_enable_width 1
#define LOCK_adc_buffer_complete_int_1_enable_addr 0x3c
#define LOCK_adc_buffer_complete_int_1_enable_pos 4
#define LOCK_adc_buffer_complete_int_1_enable_width 1
#define LOCK_fail_int_clear_addr 0x40
#define LOCK_fail_int_clear_pos 0
#define LOCK_fail_int_clear_width 1
#define LOCK_warn_int_clear_addr 0x40
#define LOCK_warn_int_clear_pos 1
#define LOCK_warn_int_clear_width 1
#define LOCK_adc_overflow_int_clear_addr 0x40
#define LOCK_adc_overflow_int_clear_pos 2
#define LOCK_adc_overflow_int_clear_width 1
#define LOCK_adc_buffer_complete_int_0_clear_addr 0x40
#define LOCK_adc_buffer_complete_int_0_clear_pos 3
#define LOCK_adc_buffer_complete_int_0_clear_width 1
#define LOCK_adc_buffer_complete_int_1_clear_addr 0x40
#define LOCK_adc_buffer_complete_int_1_clear_pos 4
#define LOCK_adc_buffer_complete_int_1_clear_width 1
#define LOCK_tx_phase_addr 0x44
#define LOCK_tx_phase_pos 0
#define LOCK_tx_phase_width 8
#define LOCK_tx_power_addr 0x48
#define LOCK_tx_power_pos 0
#define LOCK_tx_power_width 8
#define LOCK_rx_gain_addr 0x4c
#define LOCK_rx_gain_pos 0
#define LOCK_rx_gain_width 8
#define LOCK_lock_attenuation_addr 0x50
#define LOCK_lock_attenuation_pos 0
#define LOCK_lock_attenuation_width 1
#define LOCK_lock_gain_hi_lo_addr 0x54
#define LOCK_lock_gain_hi_lo_pos 0
#define LOCK_lock_gain_hi_lo_width 1
#define LOCK_locked_addr 0x58
#define LOCK_locked_pos 0
#define LOCK_locked_width 1
#define LOCK_seq_gen_enable_addr 0x5c
#define LOCK_seq_gen_enable_pos 0
#define LOCK_seq_gen_enable_width 1
#define LOCK_tx_start_addr 0x60
#define LOCK_tx_start_pos 0
#define LOCK_tx_start_width 32
#define LOCK_tx_stop_addr 0x64
#define LOCK_tx_stop_pos 0
#define LOCK_tx_stop_width 32
#define LOCK_rx_start_addr 0x68
#define LOCK_rx_start_pos 0
#define LOCK_rx_start_width 32
#define LOCK_rx_stop_addr 0x6c
#define LOCK_rx_stop_pos 0
#define LOCK_rx_stop_width 32
#define LOCK_ctc_period_addr 0x70
#define LOCK_ctc_period_pos 0
#define LOCK_ctc_period_width 16
#define LOCK_num_ctc_pulses_addr 0x74
#define LOCK_num_ctc_pulses_pos 0
#define LOCK_num_ctc_pulses_width 16
#define LOCK_channel_phase_addr 0x78
#define LOCK_channel_phase_pos 0
#define LOCK_channel_phase_width 16
#define LOCK_channel_pulse_width_addr 0x7c
#define LOCK_channel_pulse_width_pos 0
#define LOCK_channel_pulse_width_width 16
#define LOCK_adc_spare_addr 0x80
#define LOCK_adc_spare_pos 0
#define LOCK_adc_spare_width 2
#define LOCK_sample_block_select_addr 0x84
#define LOCK_sample_block_select_pos 0
#define LOCK_sample_block_select_width 1
#define LOCK_adc_count0_addr 0x88
#define LOCK_adc_count0_pos 0
#define LOCK_adc_count0_width 13
#define LOCK_adc_count1_addr 0x8c
#define LOCK_adc_count1_pos 0
#define LOCK_adc_count1_width 13
#define LOCK_dds_reset_addr 0x90
#define LOCK_dds_reset_pos 0
#define LOCK_dds_reset_width 1
#define LOCK_dds_update_addr 0x94
#define LOCK_dds_update_pos 0
#define LOCK_dds_update_width 1
#define LOCK_dds_update_en_addr 0x98
#define LOCK_dds_update_en_pos 0
#define LOCK_dds_update_en_width 1
#define LOCK_dds_update_out_addr 0x9c
#define LOCK_dds_update_out_pos 0
#define LOCK_dds_update_out_width 1
#define LOCK_la_trigger_select_addr 0xa0
#define LOCK_la_trigger_select_pos 0
#define LOCK_la_trigger_select_width 7
#define LOCK_la_trigger_polarity_addr 0xa0
#define LOCK_la_trigger_polarity_pos 7
#define LOCK_la_trigger_polarity_width 2
#define LOCK_la_trigger_arm_addr 0xa0
#define LOCK_la_trigger_arm_pos 9
#define LOCK_la_trigger_arm_width 1
#define LOCK_la_force_trigger_addr 0xa0
#define LOCK_la_force_trigger_pos 10
#define LOCK_la_force_trigger_width 1
#define LOCK_la_trigger_leadin_addr 0xa0
#define LOCK_la_trigger_leadin_pos 11
#define LOCK_la_trigger_leadin_width 10
#define LOCK_la_sample_divisor_addr 0xa0
#define LOCK_la_sample_divisor_pos 21
#define LOCK_la_sample_divisor_width 8
#define LOCK_la_start_data_addr 0xa4
#define LOCK_la_start_data_pos 0
#define LOCK_la_start_data_width 10
#define LOCK_la_triggered_addr 0xa4
#define LOCK_la_triggered_pos 10
#define LOCK_la_triggered_width 1
#define LOCK_la_checksum_addr 0xa8
#define LOCK_la_checksum_pos 0
#define LOCK_la_checksum_width 16
#define LOCK_la_address_addr 0xac
#define LOCK_la_address_pos 0
#define LOCK_la_address_width 12
#define LOCK_la_data_addr 0xb0
#define LOCK_la_data_pos 0
#define LOCK_la_data_width 32
#define LOCK_real_data0_addr 0x8000
#define LOCK_real_data0_pos 0
#define LOCK_real_data0_width 16
#define LOCK_imaginary_data0_addr 0x8000
#define LOCK_imaginary_data0_pos 16
#define LOCK_imaginary_data0_width 16
#define LOCK_real_data1_addr 0x10000
#define LOCK_real_data1_pos 0
#define LOCK_real_data1_width 16
#define LOCK_imaginary_data1_addr 0x10000
#define LOCK_imaginary_data1_pos 16
#define LOCK_imaginary_data1_width 16
#endif
