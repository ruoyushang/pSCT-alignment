/*
 * Compose and decode messages for TLCx5xx 4/8-channel ADCs
 */

#ifndef TLC3548_ADC_HPP
#define TLC3548_ADC_HPP

#include <stdint.h>
#define NBIT 14

namespace TLC3548
{
    enum OutputFormat { OF_BOB, OF_USB, OF_BTC };
    enum SamplePeriod { SP_LONG, SP_SHORT };
    enum ReferenceSelect { RS_INTERNAL, RS_EXTERNAL };
    enum ConversionClock { CC_INTERNAL, CC_SCLK };
    enum InputMode { IM_SINGLE_ENDED, IM_PSEUDO_DIFFERENTIAL };
    enum ConversionMode { CM_ONE_SHOT, CM_REPEAT, CM_SWEEP, CM_REPEAT_SWEEP };
    enum SweepSequence { SS_01234567, SS_02460246, SS_00224466, SS_02020202 };
    enum PinFunction { PF_INT_BAR, PF_EOC };
    enum TriggerLevel { TL_FULL, TL_75PC, TL_50PC, TL_25PC };

    uint32_t codeCommand(uint32_t cmd, uint32_t data = 0);
    uint32_t codeSelectChannel(unsigned ichan);
    float voltData(const uint32_t data, const float full_volt = 5.0);

    uint32_t codeSWPowerDown();
    uint32_t codeSelectRefMid();
    uint32_t codeSelectRefM();
    uint32_t codeSelectRefP();
    uint32_t codeSelect(unsigned ichan);
    uint32_t codeReadFIFO();
    uint32_t codeConfigDefault();

    uint32_t fullScaleUSB();
    uint32_t decodeUSB(uint32_t data);
    int32_t decodeBOB(uint32_t data);
    int32_t decodeBTC(uint32_t data);
    float fracData(const uint32_t data);
    float fracUSB(const uint32_t data);
    float voltUSB(const uint32_t data, const float full_volt = 5.0);

    uint32_t codeInitialize();
    uint32_t codeConfig(
            SamplePeriod    sp = SP_SHORT,
            ReferenceSelect rs = RS_EXTERNAL,
            ConversionClock cc = CC_INTERNAL,
            ConversionMode  cm = CM_ONE_SHOT,
            SweepSequence   ss = SS_01234567,
            InputMode       im = IM_SINGLE_ENDED,
            OutputFormat    of = OF_BOB,
            PinFunction     pf = PF_EOC,
            TriggerLevel    tl = TL_50PC);
};

#endif // ndef TLC3548_ADC_HPP
