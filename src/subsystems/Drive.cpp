#include "subsystems/Drive.h"

#include <cmath>

#include "util/PidTuner.h"

Drive::Drive(Victor* leftVictorA, Victor* leftVictorB, Victor* rightVictorA, Victor* rightVictorB,
       Solenoid* shiftSolenoid, DoubleSolenoid* pizzaWheelSolenoid, DoubleSolenoid* brakeSolenoid,  Encoder* leftEncoder,
       Encoder* rightEncoder, Gyro* gyro, DigitalInput* bumpSensor) {
  constants_ = Constants::GetInstance();
  leftDriveMotorA_ = leftVictorA;
  leftDriveMotorB_ = leftVictorB;
  rightDriveMotorA_ = rightVictorA;
  rightDriveMotorB_ = rightVictorB;
  shiftSolenoid_ = shiftSolenoid;
  pizzaWheelSolenoid_ = pizzaWheelSolenoid;
  brakeSolenoid_ = brakeSolenoid;
  SetBrakeOn(false);
  SetHighGear(true); // Default to high gear
  leftDriveEncoder_ = leftEncoder;
  rightDriveEncoder_ = rightEncoder;
  ResetEncoders();
  gyro_ = gyro;
  gyro_->Reset();
  bumpSensor_ = bumpSensor;
  lcd_ = DriverStationLCD::GetInstance();
  prevAngularPower_ = 0.0;
}

void Drive::SetLinearPower(double left, double right) {
  double linearLeft=Linearize(left);
  double linearRight=Linearize(right);
  linearLeft = (linearLeft > 1.0) ? 1.0 : (linearLeft < -1.0) ? -1.0 : linearLeft;
  linearRight = (linearRight > 1.0) ? 1.0 : (linearRight < -1.0) ? -1.0 : linearRight;
  SetPower(linearLeft, linearRight);
}

double Drive::GetLeftEncoderDistance() {
  // Number of clicks read by encoder / number of clicks per rotation * gear ratio from encoder to wheel *
  // wheel circumference

  // Don't have current specs now, just return encoder rotations
  return -leftDriveEncoder_->Get() / 256.0 * 3.5 * 3.14159265;
}

double Drive::GetRightEncoderDistance() {
  // Number of clicks read by encoder / number of clicks per rotation * gear ratio from encoder to wheel *
  // wheel circumference

  // Don't have current specs now, just return encoder rotations
  return rightDriveEncoder_->Get() / 256.0 * 3.5 * 3.14159265;
}

void Drive::ResetEncoders() {
  leftDriveEncoder_->Reset();
  rightDriveEncoder_->Reset();
}

void Drive::SetHighGear(bool highGear) {
  shiftSolenoid_->Set(!highGear);
}

void Drive::SetPizzaWheelDown(bool down) {
  if (down) {
    pizzaWheelSolenoid_->Set(DoubleSolenoid::kForward);
  } else {
    pizzaWheelSolenoid_->Set(DoubleSolenoid::kReverse);
  }
}

void Drive::SetBrakeOn(bool on) {
  if (on) {  
    brakeSolenoid_->Set(DoubleSolenoid::kForward);
  } else { 
    brakeSolenoid_->Set(DoubleSolenoid::kReverse);
  }
}

bool Drive::GetBrakeOn() {
  return (brakeSolenoid_->Get() == DoubleSolenoid::kForward);
}

bool Drive::GetPizzaUp() {
  return !(pizzaWheelSolenoid_->Get() == DoubleSolenoid::kForward);
}

double Drive::GetGyroAngle() {
  return gyro_->GetAngle();
}

void Drive::ResetGyro() {
  gyro_->Reset();
}

int Drive::GetBumpSensorValue() {
  return bumpSensor_->Get();
}

void Drive::SetPower(double left, double right) {
  left = PwmLimit(left);
  right = PwmLimit(right);
  if (GetBrakeOn()) {
    left = right = 0;
  }
  leftDriveMotorA_->Set(left);
  leftDriveMotorB_->Set(-left); //reverse 550
  rightDriveMotorA_->Set(-right);
  rightDriveMotorB_->Set(right); //reverse 550s
}

double Drive::Linearize(double x) {
  if (fabs(x) < 0.01 ) {
      x = 0.0;
  }
  if (x > 0.0) {
    return constants_->linearCoeffA * pow(x, 4) + constants_->linearCoeffB * pow(x, 3) +
        constants_->linearCoeffC * pow(x, 2) + constants_->linearCoeffD * x + constants_->linearCoeffE;
  } else if (x < 0.0) {
    // Rotate the linearization function by 180.0 degrees to handle negative input.
    return -Linearize(-x);
  } else {
    return 0.0;
  }
}

void Drive::CheesyDrive(double throttle, double wheel, bool quickTurn) {
  double angularPower = 0.0;
  double overPower = 0.0;
  double sensitivity = 1.0;
  double rPower = 0.0;
  double lPower = 0.0;
  
  static double fpga = Timer::GetFPGATimestamp();
    static double prevGyro = gyro_->GetAngle();
    double curfpga = Timer::GetFPGATimestamp();
    double curGyro = gyro_->GetAngle();
    double dt = curfpga - fpga;
    double dtheta = curGyro - prevGyro;
    double wubbleu = dtheta / dt;
    fpga = curfpga;
    prevGyro = curGyro;
    //PidTuner::PushData(wubbleu, 0, 0);

  if (!shiftSolenoid_->Get()) //high gear
    sensitivity = constants_->turnSensHigh;
  else {
    sensitivity = constants_->turnSensLow;
    printf("low gear....\n");
  }

  if (quickTurn) {
    overPower = 1.0;
    sensitivity = 1.0;
    angularPower = wheel;
  }
  else {
    overPower = 0.0;
    angularPower = fabs(throttle) * wheel * sensitivity;
  }
  
  if (!quickTurn) {
    angularPower -= wubbleu * constants_->inertiaGain;
  }

  rPower = lPower = throttle;
  lPower += angularPower;
  rPower -= angularPower;

  if (lPower > 1.0) {
    rPower -= overPower * (lPower - 1.0);
    lPower = 1.0;
  }
  else if (rPower > 1.0) {
    lPower -= overPower * (rPower - 1.0);
    rPower = 1.0;
  }
  else if (lPower < -1.0) {
    rPower += overPower * (-1.0 - lPower);
    lPower = -1.0;
  }
  else if (rPower < -1.0) {
    lPower += overPower * (-1.0 - rPower);
    rPower = -1.0;
  }
  
  if(throttle == 0 && !quickTurn) {
	  SetLinearPower(0.0, 0.0);
  }
  
  printf("t: %f l: %f r: %f\n", throttle, lPower, rPower);

  //  printf("ts: %f | lp: %f\nrp: %f\n\n", sensitivity, lPower, rPower);
  SetLinearPower(lPower, rPower);
}

