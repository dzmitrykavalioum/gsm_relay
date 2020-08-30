/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "usart_ring.h"
#include "gsm.h"

volatile uint8_t flag = 0;

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */
static void clear_string(char *src)
{
	char *dst = NULL;
	if(!src) return;
	uint8_t i = 0;

	for(dst = src; *src; src++)
	{
		if(i < 2 && (*src == '\n' || *src == '\r'))
		{
			i++;
			continue;
		}
		else if(*src == '\n' || *src == '\r') *src = ' ';

		*dst++ = *src;
	}

	*dst = 0;
}

///////////////////// колбек таймера //////////////////////
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM4)
	{
		flag = 1;
	}
}

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */
  HAL_Delay(15000); // задержка чтоб модем успел раздуплиться, если его включение происходит вместе с включением МК

    __HAL_UART_ENABLE_IT(GSM, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(DEBUG, UART_IT_RXNE);
 

    set_comand(ATCPAS);  // проверка статуса модема
      set_comand(ATCREG);  // проверка регистрации в сети - должен вернуть  +CREG: 0,1
      set_comand(ATCLIP1); // включить АОН
      set_comand(ATE);     // отключить «эхо»
      set_comand(ATS);     // поднимать трубку только "вручную"
      set_comand(ATDDET);  // включить DTMF
      //set_comand(ATCCLKK); // установить дату/время

      /////////////////// настройки для работы с sms ////////////////
      set_comand(ATCMGF);    // устанавливает текстовый режим смс-сообщения
      set_comand(ATCPBS);    // открывает доступ к данным телефонной книги SIM-карты
      set_comand(ATCSCS);    // кодировка текста - GSM
      set_comand(ATCNMI);    // настройка вывода смс в консоль

      //////////////////// различная инфа /////////////////////
      set_comand(ATIPR);       // скорость usart'a модема
      set_comand(ATI);         // название и версия модуля
      set_comand(ATCGSN);      // считывание IMEI из EEPROM
      set_comand(ATCSPN);      // оператор сети

      HAL_TIM_Base_Start_IT(&htim4);

      char buf[GSM_RX_BUFFER_SIZE] = {0,};
      char str[GSM_RX_BUFFER_SIZE] = {0,};

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
//      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
//      HAL_Delay(1000);
//      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,GPIO_PIN_SET);
  while (1)
  {
    /* USER CODE END WHILE */
	  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	  HAL_Delay(50);
    /* USER CODE BEGIN 3 */
	  if(flag == 1)
	  	{
	  		flag = 0;
	  		//get_date_time(); // будет раз в секунду выводить время
	  	}


	  	if(gsm_available()) //если модуль что-то прислал
	  	{
	  		uint16_t i = 0;
	  		uint8_t fdbg = 1;
	  		memset(buf, 0, GSM_RX_BUFFER_SIZE);
	  		HAL_Delay(50);

	  		while(gsm_available())
	  		{
	  			buf[i++] = gsm_read();
	  			if(i > GSM_RX_BUFFER_SIZE - 1) break;
	  			HAL_Delay(1);
	  		}

	  		clear_string(buf); // очищаем строку от символов \r и \n

	  		/////////////////// НАЧИНАЕМ РАСПОЗНАВАТЬ ЧТО ПРИСЛАЛ МОДУЛЬ /////////////////////
	  		if(strstr(buf, "RING") != NULL) // ЕСЛИ ЭТО ЗВОНОК
	  		{
	  			if(strstr(buf, "+375") != NULL) // если звонит нужный номер
	  			{
	  				// что-то делаем
	  				HAL_UART_Transmit(DEBUG, (uint8_t*)"My number\n", strlen("My number\n"), 1000);
	  				//incoming_call(); // можно принять звонок


	  				HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_SET);
	  				disable_connection(); // сброс соединения
	  				HAL_Delay(3000);
	  				HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET);
	  					  				  				  			  				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	  					  				  				  			  				HAL_Delay(2000);

	  			}
	  			else
	  			{
	  				HAL_UART_Transmit(DEBUG, (uint8_t*)"Unknow number\n", strlen("Unknow number\n"), 1000);
	  				disable_connection(); // сброс соединения
	  			}
	  		}
	  		else if(strstr(buf, "+CMT:") != NULL) // ЕСЛИ ЭТО SMS
	  		{
	  			if(strstr(buf, "+375") != NULL) // проверяем от кого смс
	  			{
	  				HAL_UART_Transmit(DEBUG, (uint8_t*)"Sms my number\n", strlen("Sms my number\n"), 1000);

	  				// что-то делаем или ищем какую-то строку, которую мы послали в смс, например слово "Hello"
	  				if(strstr(buf, "Hello") != NULL)
	  				{
	  					HAL_UART_Transmit(DEBUG, (uint8_t*)"Reciv Hello\n", strlen("Reciv Hello\n"), 1000);
	  					// что-то делаем


	  				}
	  				else if(strstr(buf, "Call") != NULL) // если прилетело слово "Call" то звоним
	  				{
	  					//call(); // номер указать в файле gsm.c
	  				}
	  				else if(strstr(buf, "Money") != NULL) // если отпрвить sms со словом "Money", то в ответ придёт смс с балансом (деньги) на модеме
	  				{
	  					balance(); // посылаем команду узнать баланс
	  				}
	  				else if(strstr(buf, "RRR") != NULL) // если отпрвить sms со словом "RRR"
	  				{
	  					HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_SET);
	  					HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	  					HAL_Delay(7000);
	  					HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET);
	  					HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

	  					//call();

	  				}
	  			}
	  			else
	  			{
	  				HAL_UART_Transmit(DEBUG, (uint8_t*)"Unknown number sms\n", strlen("Unknow number sms\n"), 1000);
	  				HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);//1
	  					  				HAL_Delay(200);
	  					  				HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	  					  				HAL_Delay(200);
	  					  				HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);//2
	  					  				HAL_Delay(200);
	  					  				HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	  					  				HAL_Delay(1000);
	  			}
	  		}
	  		else if(strstr(buf, "+CUSD") != NULL)  // ЕСЛИ ЭТО СТРОКА С БАЛАНСОМ
	  		{
	  			char *p = NULL;

	  			if((p = strstr(buf, "Balance")) != NULL) // ищем слово "Balance"
	  			{
	  				// отправляем смс с балансом на указанный телефон, укажите нужный номер и раскомментируйте этот блок
	  				snprintf(str, GSM_RX_BUFFER_SIZE, "AT+CMGS=\"+375336675442\"\r\n"); // номер
	  				HAL_UART_Transmit(GSM, (uint8_t*)str, strlen(str), 1000);
	  				HAL_Delay(100);
	  				snprintf(str, GSM_RX_BUFFER_SIZE, "%s", p); // текст смс
	  				HAL_UART_Transmit(GSM, (uint8_t*)str, strlen(str), 1000);
	  				p = 0;
	  				HAL_Delay(100);
	  				snprintf(str, GSM_RX_BUFFER_SIZE, "%c", (char)26); // символ ctrl-z
	  				HAL_UART_Transmit(GSM, (uint8_t*)str, strlen(str), 1000);
	  				// блок закомментирован чтоб модуль не слал смски пока тестируете
	  			}
	  		}
	  		else if(strstr(buf, "+DTMF") != NULL)  //ЕСЛИ ЭТО DTMF СИГНАЛ
	  		{
	  			if(strstr(buf, "0") != NULL) // если пришёл сигнал кнопки 0
	  			{
	  				// что-то делаем
	  				HAL_UART_Transmit(DEBUG, (uint8_t*)"DTMF Button 0\n", strlen("DTMF Button 0\n"), 1000);
	  			}
	  			else if(strstr(buf, "1") != NULL) // если пришёл сигнал кнопки 1
	  			{
	  				// что-то делаем
	  				HAL_UART_Transmit(DEBUG, (uint8_t*)"DTMF Button 1\n", strlen("DTMF Button 1\n"), 1000);
	  			}
	  			// и т.д.

	  			disable_connection(); // разрываем соединение, или не разрываем (в зависимости от того, что вам нужно)
	  		}
	  		else if(strstr(buf, "+CCLK") != NULL)  // ЕСЛИ ЭТО ДАТА/ВРЕМЯ
	  		{
	  			replac_string(buf);
	  			char res[32] = {0,};

	  			for(uint8_t i = 0; i < GSM_RX_BUFFER_SIZE; i++)
	  			{
	  				if(buf[i] == '"')
	  				{
	  					i++;
	  					for(uint8_t j = 0; j < 20; i++, j++)
	  					{
	  						if(buf[i] == '+')
	  						{
	  							buf[i] = 0;
	  							break;
	  						}

	  						if(buf[i] == ',') buf[i] = ' ';
	  						res[j] = buf[i];
	  					}

	  					break;
	  				}
	  			}

	  			snprintf(str, GSM_RX_BUFFER_SIZE, "DateTime %s\n", res);
	  			HAL_UART_Transmit(DEBUG, (uint8_t*)str, strlen(str), 1000);
	  			fdbg = 0;
	  		}

	  		if(fdbg)
	  		{
	  			snprintf(str, GSM_RX_BUFFER_SIZE, "%s\n", buf);
	  			HAL_UART_Transmit(DEBUG, (uint8_t*)str, strlen(str), 1000);
	  		}
	  	}


	  	////////////////////////////////////// DEBUG ////////////////////////////////////////
	  	if(dbg_available()) //если послали в терминал какую-то команду, то она перенаправиться в модем
	  	{
	  		uint16_t i = 0;
	  		memset(buf, 0, GSM_RX_BUFFER_SIZE);

	  		while(dbg_available())
	  		{
	  			buf[i++] = dbg_read();
	  			if(i > GSM_RX_BUFFER_SIZE - 1) break;
	  			HAL_Delay(1);
	  		}

	  		clear_string(buf);
	  		snprintf(str, GSM_RX_BUFFER_SIZE, "%s\r\n", buf);
	  		HAL_UART_Transmit(GSM, (uint8_t*)str, strlen(str), 1000);
	  	}

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
