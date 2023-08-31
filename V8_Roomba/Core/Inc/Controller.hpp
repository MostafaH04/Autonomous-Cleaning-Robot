#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "MotorControl.hpp"
#include "MPU_6050_Driver.hpp"
#include "usart.h"

#define CONNECTION_TIMEOUT 2000 // 10 seconds

enum Robot_State {
  START_UP,
  CALIBRATION,
  // add more as needed
  OPERATING,
  FAILURE,
  IDLE,
  SHUTDOWN
};

class Controller
{
  private:  
    UART_HandleTypeDef *huart;
    Chasis *chasis;
    MPU_6050 *imu_1, *imu_2;
    IMU_Data *imuData1, *imuData2;

    struct Command{
      bool omniDrive{false};
      bool shutDown{false};
      bool failure{false};
      bool operating{false};
      float angle{0};
      float speed{0}; 
      uint32_t time_ms{0};
    };

    Command currentCommand;

    Robot_State controllerState{START_UP};

    uint32_t time_ms;
    uint32_t checkVal;
	uint32_t checkVal2;

    uint8_t receiveBuffer[10] = {0};
    uint8_t transmitBuffer[25] = {0};  

    bool systemStarted{false};
    bool systemCalibrated{false};

    void startUpState();
    void calibrationState();
    void operatingState();
    void idleState();
    void failState();
    void shutdownState();

    void transmitIMU(IMU_Data *imudata, bool second);

  public:
    Controller(UART_HandleTypeDef * huart_in, Chasis *chasis_in,
      MPU_6050 *imu1_in, MPU_6050 *imu2_in);

    void loop();
    void receiveCommands();
    void trasnmitCommands();
    void parseReceived();
    void prepTransmit();

};

#endif
