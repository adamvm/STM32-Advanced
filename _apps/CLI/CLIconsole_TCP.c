/**
  ******************************************************************************
  * File Name          : CLIconsole_TCP.c
  * Description        : Code for serial driver for CLI UART application
  ******************************************************************************
  */

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* OS includes. */
#include "cmsis_os.h"

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

/* lwIP includes. */
#include "lwip/api.h"

struct netconn *TelNetConn = NULL;

/*
 * The task that runs FreeRTOS+CLI.
 */
static void prvTCPCommandInterpreterTask(void const *pvParameters) {
  char *pcOutputString;
  char cInputString[cmdMAX_INPUT_SIZE], cLastInputString[cmdMAX_INPUT_SIZE];
  BaseType_t xReturned;
  /* Obtain the address of the output buffer.
    Note there is no mutual exclusion on this buffer as it is assumed only one
    command console interface will be used at any one time. */
  pcOutputString = FreeRTOS_CLIGetOutputBuffer();
  memset(cInputString, 0x00, cmdMAX_INPUT_SIZE);
  // store help command in last input string
  sprintf(cLastInputString, "help");
  static struct netconn *conn;
  static struct netbuf *buf;
  void *data;
  u16_t len;
  /* Obtain the address of the output buffer.
    Note there is no mutual exclusion on this buffer as it is assumed only one
    command console interface will be used at any one time. */
  pcOutputString = FreeRTOS_CLIGetOutputBuffer();
  memset(cInputString, 0x00, cmdMAX_INPUT_SIZE);
  // store help command in last input string
  sprintf(cLastInputString, "help");

  /* Attempt to open the socket.  The port number is passed in the task
      parameter.  The strange casting is to remove compiler warnings on 32-bit
      machines.  NOTE:  The FREERTOS_SO_REUSE_LISTEN_SOCKET option is used,
      so the listening and connecting socket are the same - meaning only one
      connection will be accepted at a time, and that xListeningSocket must
      be created on each iteration. */
  /* Nothing for this task to do if the socket cannot be created. */

  /* Create a new connection identifier. */
  if (NULL != (conn = netconn_new(NETCONN_TCP))) {
    /* Bind connection to well known port number 23 (Telnet).
    Telnet � ����������� ��� �������� ��������� ��������� � ��������������� ���� (wIkI). */
    if (ERR_OK == netconn_bind(conn, NULL, 23)) {
      /* Put the connection into LISTEN state */
      netconn_listen(conn);

      for (;;) {
        /* Wait for an incoming connection and Process the new connection. */
        if (ERR_OK == netconn_accept(conn, &TelNetConn))  {
//          BSP_AUDIO_Play(SND_ETHCON);
          /* Send the welcome message. */
          netconn_write(TelNetConn,
                        pcNewLine,
                        strlen(pcNewLine),
                        NETCONN_NOCOPY);
          netconn_write(TelNetConn,
                        pcWelcomeMessage,
                        strlen(pcWelcomeMessage),
                        NETCONN_NOCOPY);

          /* Recieve from socket */
          while (netconn_recv(TelNetConn, &buf) == ERR_OK) {
            do {
              netbuf_data(buf, &data, &len);

              // if "\r\n"
              if (len == 2) {
//                BSP_AUDIO_Play(SND_GET_CMD);
                /* Just to space the output from the input. */
                netconn_write(TelNetConn,
                              pcNewLine,
                              strlen(pcNewLine),
                              NETCONN_NOCOPY);

                /* See if the command is empty,
                indicating that the last command is to be executed again. */
                if (cInputString[0] == '\0') {
                  /* Copy the last command back into the input string. */
                  strcpy(cInputString, cLastInputString);
                }

                /* Process the input string received prior to the newline. */
                do {
                  /* Get the next output string from the command interpreter. */
                  xReturned = FreeRTOS_CLIProcessCommand(cInputString,
                                                         pcOutputString,
                                                         configCOMMAND_INT_MAX_OUTPUT_SIZE);
                  /* Send the output generated by the command's implementation. */
                  netconn_write(TelNetConn,
                                pcOutputString,
                                strlen(pcOutputString),
                                NETCONN_COPY);
                  /* Until the command does not generate any more output. */
                } while (xReturned != pdFALSE);

                /* Transmit a spacer to make the console easier to read. */
                netconn_write(TelNetConn,
                              pcEndOfOutputMessage,
                              strlen(pcEndOfOutputMessage),
                              NETCONN_NOCOPY);
                /* All the strings generated by the command
                  processing have been sent.  Clear the input string
                  ready to receive the next command.  Remember the
                  previous command so it can be executed again by
                  pressing [ENTER]. */
                strcpy(cLastInputString, cInputString);
                // clear input string before next input from user
                memset(cInputString, 0x00, cmdMAX_INPUT_SIZE);
              } else {
                /* Copy data to local buffer */
                memcpy(cInputString, data, len);
              }
            } while (netbuf_next(buf) >= 0);

            netbuf_delete(buf);
          }

          /* Close connection and discard connection identifier. */
          netconn_close(TelNetConn);
          /* delete connection */
          netconn_delete(TelNetConn);
        }
      }
    }
  }
}
/*-----------------------------------------------------------------------------*/

void vStartTCPCommandInterpreterTask() {
  /* Create that task that handles the console itself.
  cli_thread     - The task that implements the command console.
  "CLI"          - Text name assigned to the task.  This is just to assist debugging. The kernel does not use this name itself.
  usStackSize    - The size of the stack allocated to the task.
  NULL           - The parameter is not used, so NULL is passed.
  uxPriority     - The priority allocated to the task.
  xCLITaskHandle - Serial drivers that use task notifications may need the CLI task's handle. */
  osThreadDef(CLI_TCP,
              prvTCPCommandInterpreterTask,
              osPriorityLow,
              0,
              DEFAULT_THREAD_STACKSIZE);
  osThreadCreate(osThread(CLI_TCP), NULL);
}
