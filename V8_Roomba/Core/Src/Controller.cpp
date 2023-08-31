#include "Controller.hpp"

Controller::Controller(UART_HandleTypeDef * huart_in, Chasis *chasis_in,
      MPU_6050 *imu1_in, MPU_6050 *imu2_in):
huart(huart_in), chasis(chasis_in), imu_1(imu1_in), imu_2(imu2_in)
{}

void Controller::startUpState()
{
  chasis->init();
  imu_1->startPins();
  imu_2->startPins();
  controllerState = CALIBRATION;
  systemStarted = true;
}

void Controller::calibrationState()
{
  imu_1->initialize();
  imu_2->initialize();
  imuData1 = imu_1->generateNewResult();
  imuData2 = imu_2->generateNewResult();
  controllerState = OPERATING;
  systemCalibrated = true;

  receiveCommands();
}

void Controller::operatingState()
{
  chasis->steer(currentCommand.angle,
                currentCommand.speed);
  // currently omniDrive is not supported
  // if (currentCommand.failure)
  //   controllerState = FAILURE;
  // else if (currentCommand.shutDown)
  //   controllerState = SHUTDOWN;
  // else if (!currentCommand.operating)
  //   controllerState = IDLE;
}

void Controller::idleState()
{
  chasis->steer(0,0);

  if (time_ms > checkVal2)
  {
    controllerState = SHUTDOWN;
  }
  else if (time_ms < checkVal)
  {
    controllerState = OPERATING;
  }
}

void Controller::failState()
{
  chasis->steer(0,0);
  if (!currentCommand.failure)
    controllerState = OPERATING;
}

void Controller::shutdownState()
{
  receiveCommands(); // attempt to restart connection
  controllerState = OPERATING;
  return;
}

void Controller::loop()
{
  time_ms = HAL_GetTick();

  imu_1->updateData();
  imu_2->updateData();

  checkVal = currentCommand.time_ms + CONNECTION_TIMEOUT;
  checkVal2 = currentCommand.time_ms + CONNECTION_TIMEOUT * 2;

  if (time_ms > checkVal && controllerState != SHUTDOWN)
  {
    controllerState = IDLE;
  }

  switch(controllerState)
  {
    case START_UP:
      startUpState();
      break;
    
    case CALIBRATION:
      calibrationState();
      break;

    case OPERATING:
      operatingState();
      break;
    
    case IDLE:
      idleState();
      break;

    case FAILURE:
      failState();
      break;
    
    case SHUTDOWN:
      shutdownState();
      break;
  }
}

void Controller::receiveCommands()
{
  HAL_UART_Receive_IT(&huart1, receiveBuffer, 5);
  trasnmitCommands();
}

void Controller::parseReceived()
{
  currentCommand.failure = (receiveBuffer[0] & 0b00001000);
  currentCommand.shutDown = !(receiveBuffer[0] & 0b00000100);
  currentCommand.operating = (receiveBuffer[0] & 0b00000010);
  currentCommand.omniDrive = (receiveBuffer[0] & 0b00000001);

  currentCommand.angle = int16_t(receiveBuffer[1] << 8 | receiveBuffer[2])/1000.0;
  currentCommand.speed = int16_t(receiveBuffer[3] << 8 | receiveBuffer[4])/1000.0;

  currentCommand.time_ms = HAL_GetTick();;
}

void Controller::prepTransmit()
{
  transmitBuffer[0] = int(systemStarted) << 1 
					            | int(systemCalibrated);
  
  int16_t accX = imuData1->accX * 1000;
  int16_t accY = imuData1->accY * 1000;
  int16_t accZ = imuData1->accZ * 1000;
  int16_t gyroX = imuData1->gyroX * 1000;
  int16_t gyroY = imuData1->gyroY * 1000;
  int16_t gyroZ = imuData1->gyroZ * 1000;


  transmitBuffer[1] = (accX & 0b1111111100000000) >> 8;
  transmitBuffer[2] = (accX & 0b0000000011111111);

  transmitBuffer[3] = (accY & 0b1111111100000000) >> 8;
  transmitBuffer[4] = (accY & 0b0000000011111111);

  transmitBuffer[5] = (accZ & 0b1111111100000000) >> 8;
  transmitBuffer[6] = (accZ & 0b0000000011111111);

  transmitBuffer[7] = (gyroX & 0b1111111100000000) >> 8;
  transmitBuffer[8] = (gyroX & 0b0000000011111111);

  transmitBuffer[9] = (gyroY & 0b1111111100000000) >> 8;
  transmitBuffer[10] = (gyroY & 0b0000000011111111);

  transmitBuffer[11] = (gyroZ & 0b1111111100000000) >> 8;
  transmitBuffer[12] = (gyroZ & 0b0000000011111111);

  accX = imuData2->accX * 1000;
  accY = imuData2->accY * 1000;
  accZ = imuData2->accZ * 1000;
  gyroX = imuData2->gyroX * 1000;
  gyroY = imuData2->gyroY * 1000;
  gyroZ = imuData2->gyroZ * 1000;

  transmitBuffer[13] = (accX & 0b1111111100000000) >> 8;
  transmitBuffer[14] = (accX & 0b0000000011111111);

  transmitBuffer[15] = (accY & 0b1111111100000000) >> 8;
  transmitBuffer[16] = (accY & 0b0000000011111111);

  transmitBuffer[17] = (accZ & 0b1111111100000000) >> 8;
  transmitBuffer[18] = (accZ & 0b0000000011111111);

  transmitBuffer[19] = (gyroX & 0b1111111100000000) >> 8;
  transmitBuffer[20] = (gyroX & 0b0000000011111111);

  transmitBuffer[21] = (gyroY & 0b1111111100000000) >> 8;
  transmitBuffer[22] = (gyroY & 0b0000000011111111);

  transmitBuffer[23] = (gyroZ & 0b1111111100000000) >> 8;
  transmitBuffer[24] = (gyroZ & 0b0000000011111111);
}

void Controller::trasnmitCommands()
{
  prepTransmit();
  HAL_UART_Transmit(&huart1, transmitBuffer, sizeof(transmitBuffer), 1000);
}

