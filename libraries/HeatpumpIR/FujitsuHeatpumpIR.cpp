#include <FujitsuHeatpumpIR.h>

FujitsuHeatpumpIR::FujitsuHeatpumpIR() : HeatpumpIR()
{
  static const char PROGMEM model[] PROGMEM = "fujitsu_awyz";
  static const char PROGMEM info[]  PROGMEM = "{\"mdl\":\"fujitsu_awyz\",\"dn\":\"Fujitsu AWYZ\",\"mT\":16,\"xT\":30,\"fs\":5}";

  _model = model;
  _info = info;
}


void FujitsuHeatpumpIR::send(IRSender& IR, byte powerModeCmd, byte operatingModeCmd, byte fanSpeedCmd, byte temperatureCmd, byte swingVCmd, byte swingHCmd)
{
  send(IR,  powerModeCmd, operatingModeCmd, fanSpeedCmd, temperatureCmd, swingVCmd, swingHCmd, false);
}


void FujitsuHeatpumpIR::send(IRSender& IR, byte powerModeCmd, byte operatingModeCmd, byte fanSpeedCmd, byte temperatureCmd, byte swingVCmd, byte swingHCmd, bool ecoModeCmd)
{
  // Sensible defaults for the heat pump mode

  byte operatingMode = FUJITSU_AIRCON1_MODE_HEAT;
  byte fanSpeed = FUJITSU_AIRCON1_FAN_AUTO;
  byte temperature = 23;
  byte swingV = FUJITSU_AIRCON1_VDIR_MANUAL;
  byte swingH = FUJITSU_AIRCON1_HDIR_MANUAL;
  byte ecoMode = FUJITSU_AIRCON1_ECO_OFF;

  if (powerModeCmd == POWER_OFF)
  {
    operatingMode = FUJITSU_AIRCON1_MODE_OFF;
  }
  else
  {
    switch (operatingModeCmd)
    {
      case MODE_AUTO:
        operatingMode = FUJITSU_AIRCON1_MODE_AUTO;
        break;
      case MODE_HEAT:
        operatingMode = FUJITSU_AIRCON1_MODE_HEAT;
        break;
      case MODE_COOL:
        operatingMode = FUJITSU_AIRCON1_MODE_COOL;
        break;
      case MODE_DRY:
        operatingMode = FUJITSU_AIRCON1_MODE_DRY;
        break;
      case MODE_FAN:
        operatingMode = FUJITSU_AIRCON1_MODE_FAN;
        // When Fujitsu goes to FAN mode, it sets the low bit of the byte with the temperature. What is the meaning of that?
       break;
    }
  }

  switch (fanSpeedCmd)
  {
    case FAN_AUTO:
      fanSpeed = FUJITSU_AIRCON1_FAN_AUTO;
      break;
    case FAN_1:
      fanSpeed = FUJITSU_AIRCON1_FAN1;
      break;
    case FAN_2:
      fanSpeed = FUJITSU_AIRCON1_FAN2;
      break;
    case FAN_3:
      fanSpeed = FUJITSU_AIRCON1_FAN3;
      break;
    case FAN_4:
      fanSpeed = FUJITSU_AIRCON1_FAN4;
      break;
  }

  if ( temperatureCmd > 15 && temperatureCmd < 31)
  {
    temperature = temperatureCmd;
  }

  // Only support 'don't move' or 'swing' at the moment, not any specific position
  if ( swingVCmd == VDIR_SWING) {
    swingV = FUJITSU_AIRCON1_VDIR_SWING;
  }

  if ( swingHCmd == HDIR_SWING) {
    swingH = FUJITSU_AIRCON1_HDIR_SWING;
  }

  if (ecoModeCmd) {
    ecoMode = FUJITSU_AIRCON1_ECO_ON;
  }

  sendFujitsu(IR, operatingMode, fanSpeed, temperature, swingV, swingH, ecoMode);
}


void FujitsuHeatpumpIR::sendFujitsu(IRSender& IR, byte operatingMode, byte fanSpeed, byte temperature, byte swingV, byte swingH, byte ecoMode)
{
  // ON, HEAT, AUTO FAN, +24 degrees
  byte FujitsuTemplate[] = { 0x14, 0x63, 0x00, 0x10, 0x10, 0xFE, 0x09, 0x30, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00 };
  //                            0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15

  byte OFF_msg[] = { 0x14, 0x63, 0x00, 0x10, 0x10, 0x02, 0xFD };
  byte checksum = 0x00;

/*

  Fujitsu does not have codes to set the air direction to any specific position, but just go to the next position:

  byte nextVerticalPosition_msg[] = { 0x14, 0x63, 0x00, 0x10, 0x10, 0x6C, 0x93 };
  byte nextHorizontalPosition_msg[] = { 0x14, 0x63, 0x00, 0x10, 0x10, 0x79,0x86 };

  These would need to be sent separately...
*/
  // Set the operatingmode on the template message
  FujitsuTemplate[9] = operatingMode;

  // Set the eco mode on the template message
  FujitsuTemplate[14] = ecoMode;

  // Set the temperature on the template message. The least significant bit should be set to '1'
  FujitsuTemplate[8] = (temperature - 16) << 4  | 0x01;

  // Set the fan speed and air direction on the template message
  FujitsuTemplate[10] = fanSpeed + swingV + swingH;

  // Calculate the checksum
  for (int i=0; i<15; i++) {
    checksum += FujitsuTemplate[i];
  }

  FujitsuTemplate[15] = (byte)(0x9E - checksum);

  if (operatingMode == FUJITSU_AIRCON1_MODE_OFF) {
    // OFF
    sendFujitsuMsg(IR, sizeof(OFF_msg), OFF_msg);
  } else {
    sendFujitsuMsg(IR, sizeof(FujitsuTemplate), FujitsuTemplate);
  }
}


void FujitsuHeatpumpIR::sendFujitsuHiPower(IRSender& IR)
{
  byte HiPower_msg[] = { 0x14, 0x63, 0x00, 0x10, 0x10, 0x39, 0xC6 };

  sendFujitsuMsg(IR, sizeof(HiPower_msg), HiPower_msg);
}


void FujitsuHeatpumpIR::sendFujitsuFilterClean(IRSender& IR)
{
  byte FilterClean_msg[] = { 0x14, 0x63, 0x00, 0x10, 0x10, 0x02, 0xFD };

  sendFujitsuMsg(IR, sizeof(FilterClean_msg), FilterClean_msg);
}


void FujitsuHeatpumpIR::sendFujitsuSuperQuiet(IRSender& IR)
{
  byte SuperQuiet_msg[] = { 0x14, 0x63, 0x00, 0x10, 0x10, 0x02, 0xFD };

  sendFujitsuMsg(IR, sizeof(SuperQuiet_msg), SuperQuiet_msg);
}


void FujitsuHeatpumpIR::sendFujitsuTestRun(IRSender& IR)
{
  byte TestRun_msg[] = { 0x14, 0x63, 0x00, 0x10, 0x10, 0x02, 0xFD };

  sendFujitsuMsg(IR, sizeof(TestRun_msg), TestRun_msg);
}


void FujitsuHeatpumpIR::sendFujitsuMsg(IRSender& IR, byte msgSize, byte *msg)
{
  // 40 kHz PWM frequency
  IR.setFrequency(40);

  // Header
  IR.mark(FUJITSU_AIRCON1_HDR_MARK);
  IR.space(FUJITSU_AIRCON1_HDR_SPACE);

  // Data
  for (byte i=0; i<msgSize; i++) {
      IR.sendIRByte(msg[i], FUJITSU_AIRCON1_BIT_MARK, FUJITSU_AIRCON1_ZERO_SPACE, FUJITSU_AIRCON1_ONE_SPACE);
  }

  // End mark
  IR.mark(FUJITSU_AIRCON1_BIT_MARK);
  IR.space(0);
}