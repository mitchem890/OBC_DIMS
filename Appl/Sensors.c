
void Sensor_Init(){
    //-------------------- sensors init start and print to both communication channels ------------------
    // That is just an example how to initialise all sensors and to print to two USART channels if needed

    //initialize Magnitometer
    if (Magnitometers_LIS3MDL_Init(LIS3MDL_MAG_I2C_ADDRESS_LOW) == E_OK)
      {
          fprintf(PAYLOAD,"MAG1_INIT_OK\r");
          fprintf(COMM  ,"MAG1_INIT_OK\r");
      }
      else{
          fprintf(PAYLOAD, "  MAG1 fail\r");
          fprintf(COMM  , "  MAG1 fail\r");
      }
      if (Magnitometers_LIS3MDL_Init(LIS3MDL_MAG_I2C_ADDRESS_HIGH) == E_OK)
      {
          fprintf(PAYLOAD,"MAG2_INIT_OK\r");
          fprintf(COMM  ,"MAG2_INIT_OK\r");
      }
      else{
          fprintf(PAYLOAD, "  MAG2 fail\r");
          fprintf(COMM  , "  MAG2 fail\r");
      }

      //Inizialize ACC Sensor 1
      if (AIS328DQ_Init(AIS328DQ_1_MEMS_I2C_ADDRESS) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"ACC1_INIT_OK\r");
        fprintf(COMM  ,"ACC1_INIT_OK\r");
      }
      else{
        I2C_Reset(&hi2c2);
      }

      //Inizialize ACC Sensor 2
      if (AIS328DQ_Init(AIS328DQ_2_MEMS_I2C_ADDRESS) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"ACC2_INIT_OK\r");
        fprintf(COMM  ,"ACC2_INIT_OK\r");
      }
      else{
        I2C_Reset(&hi2c2);
      }

	//Inizialize GYR Sensor X
      if (ADIS16265_Init(PAN_X_M) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"GYR%u_INIT_OK\r", GYR_1);
        fprintf(COMM  ,"GYR%u_INIT_OK\r", GYR_1);
      }
      else{
        fprintf(PAYLOAD,"GYR%u_INIT_FAIL\r", GYR_1);
        fprintf(COMM  ,"GYR%u_INIT_FAIL\r", GYR_1);
      }

      //Inizialize GYR Sensor Y
      if (ADIS16265_Init(PAN_Y_M) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"GYR%u_INIT_OK\r", GYR_2);
        fprintf(COMM  ,"GYR%u_INIT_OK\r", GYR_2);
      }
      else{
        fprintf(PAYLOAD,"GYR%u_INIT_FAIL\r", GYR_2);
        fprintf(COMM  ,"GYR%u_INIT_FAIL\r", GYR_2);
      }

      //Inizialize GYR Sensor Z
      if (ADIS16265_Init(PAN_Z_M) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"GYR%u_INIT_OK\r", GYR_3);
        fprintf(COMM  ,"GYR%u_INIT_OK\r", GYR_3);
      }
      else{
        fprintf(PAYLOAD,"GYR%u_INIT_FAIL\r", GYR_3);
        fprintf(COMM  ,"GYR%u_INIT_FAIL\r", GYR_3);
      }

}

void Magnetorquer_Disable(){
	/* Set PWM of the magnetorquer to 0% (i.e. OFF) */
      if (SetMagnetorque(PAN_X_M, 0, 1) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"TRQ%u_INIT_OK 0%%\r", TRQ_1);
        fprintf(COMM  ,"TRQ%u_INIT_OK 0%%\r", TRQ_1);
      }

      /* Set PWM of the magnetorquer to 0% (i.e. OFF) */
      if (SetMagnetorque(PAN_Y_M, 0, 1) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"TRQ%u_INIT_OK 0%%\r", TRQ_2);
        fprintf(COMM  ,"TRQ%u_INIT_OK 0%%\r", TRQ_2);
      }

      /* Set PWM of the magnetorquer to 0% (i.e. OFF) */
      if (SetMagnetorque(PAN_Z_M, 0, 1) == SEN_SUCCESS)
      {
        fprintf(PAYLOAD,"TRQ%u_INIT_OK 0%%\r", TRQ_3);
        fprintf(COMM  ,"TRQ%u_INIT_OK 0%%\r", TRQ_3);
      }


}
void Toggle_XCAM_Power(XCAM_PowerOff, XCAM_PowerOn){
 // To turn off --> 5v then 3v 
        // To turn on  --> 3v then 5v
        if (XCAM_PowerOff == 1 ) {
          fprintf(PAYLOAD,"\r\tTurning off LUP in AppTasks\r");
          EPS_write(6,1);  //  --> turn off LUP 5v
          XCAM_WaitSeconds(1,0);
          EPS_write(5,1);  //  --> turn off LUP 3.3v
          // wait for everything to settle
          XCAM_WaitSeconds(30,1);
          EPS_check(0,1);
        }
        if (XCAM_PowerOn  == 1 ) {
          fprintf(PAYLOAD,"\r\tTurning on  LUP in AppTasks\r");
          EPS_write(5,0);  //  --> turn  on LUP 3.3v
          XCAM_WaitSeconds(5,1);
          EPS_write(6,0);  //  --> turn  on LUP 5v
          XCAM_WaitSeconds(5,1);
          EPS_check(0,1);
        }
}

