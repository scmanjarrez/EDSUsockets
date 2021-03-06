#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define ENV 5
#define MAX_REC_SZ 512

typedef struct topic_message
{
  int op;
  char tp_nam[64];
  char tp_val[64];
}TOPIC_MSG;

size_t serialize(const TOPIC_MSG *topic, unsigned char **buf);
TOPIC_MSG *deserialize(const unsigned char *buf, const size_t bufSz);

int main()
{
  TOPIC_MSG msg;
  bzero((char*)&msg, sizeof(TOPIC_MSG));
  
  char *tema = "elcolor";
  char *valor = "es bueno";
  msg.op = ENV;

  sprintf(msg.tp_nam, "%s", tema);
  sprintf(msg.tp_val, "%s", valor);
  
  printf("Sin serializar: El op=%d, el tema=%s, el valor=%s\n", msg.op, msg.tp_nam, msg.tp_val);
  size_t msg_sz;
  unsigned char *buf = 0;
  msg_sz = serialize(&msg, &buf);
  
  ssize_t tam;
  int fd[2];
  pipe(fd);
  int j;
  
  tam=write(fd[1], buf, msg_sz);

  unsigned char buff[4096] = {0};
  unsigned char resp[MAX_REC_SZ];
  int offset = 0;

  tam=read(fd[0], (void*)resp, MAX_REC_SZ);
  memcpy(buff + offset, &buf, (size_t)tam);
  offset+=tam;

  TOPIC_MSG *msgI;
  msgI = deserialize(buf, (size_t)offset);

  printf("Después de deserializar: El op=%d, el tema=%s, el valor=%s\n", msgI->op, msgI->tp_nam, msgI->tp_val);

  close(fd[0]);
  close(fd[1]);

  return 0;
}

size_t serialize(const TOPIC_MSG *topic, unsigned char **buf)
{

  size_t tp_nam_sz, tp_val_sz, tp_sz;
  tp_nam_sz = topic->tp_nam ? strlen(topic->tp_nam) : 0;
  tp_val_sz   = topic->tp_val ? strlen(topic->tp_val) : 0;
  tp_sz = sizeof(uint32_t) + sizeof(uint32_t) + tp_nam_sz + sizeof(uint32_t) + tp_val_sz;

  if((*buf = calloc(1, tp_sz))==NULL)
    {
      perror("Error");
      return -1;
    }

  size_t offset = 0;
  unsigned int tmp;

  // op
  tmp = htonl(topic->op);
  memcpy(*buf + offset, &tmp, sizeof(tmp));
  offset += sizeof(tmp);

  // tamaño de tp_nam y tp_nam
  tmp = htonl(tp_nam_sz);
  memcpy(*buf + offset, &tmp, sizeof(tmp));
  offset += sizeof(tmp);
  memcpy(*buf + offset, topic->tp_nam, tp_nam_sz);
  offset += tp_nam_sz;

  // tamaño de tp_val y tp_val
  tmp = htonl(tp_val_sz);
  memcpy(*buf + offset, &tmp, sizeof(tmp));
  offset += sizeof(tmp);
  memcpy(*buf + offset, topic->tp_val, tp_val_sz);

  return tp_sz;
}


TOPIC_MSG *deserialize(const unsigned char *buf, const size_t bufSz)
{
  /* 4 bytes de int, 4 por cada size_t y los string de tamaño 0*/
  static const size_t MIN_BUF_SZ = 12;

  TOPIC_MSG *topic;

  if (buf && bufSz < MIN_BUF_SZ)
    {
      fprintf(stderr, "El tamaño del buffer es menor que el mínimo\n");
      return NULL;
    }

  if((topic = calloc(1, sizeof(TOPIC_MSG)))==NULL)
    {
      perror("Error");
      return NULL;
    }

  size_t tmp = 0, offset = 0;

  // obtenemos op
  memcpy(&tmp, buf + offset, sizeof(tmp));
  tmp = ntohl(tmp);
  memcpy(&topic->op, &tmp, sizeof(topic->op));
  offset  += sizeof(uint32_t);

  // obtenemos el tamaño de tp_nam
  memcpy(&tmp, buf + offset, sizeof(tmp));
  tmp = ntohl(tmp);
  offset  += sizeof(uint32_t);

  if (tmp > 0)
    {
      // obtenemos tp_nam
      //topic->tp_nam = calloc(1, tmp + 1);
      memcpy(topic->tp_nam, buf + offset, tmp);

      if(strlen(topic->tp_nam) != (size_t)tmp)
	{
	  fprintf(stderr, "El tamaño del nombre del tema no coincide con el enviado\n");
	  return NULL;
	}
      offset  += tmp;
    }

  // obtenemos el tamaño de tp_val
  memcpy(&tmp, buf + offset, sizeof(tmp));
  tmp = ntohl(tmp);
  offset  += sizeof(uint32_t);

  if (tmp > 0)
    {
      // obtenemos tp_val
      //topic->tp_val = calloc(1, tmp + 1);
      memcpy(topic->tp_val, buf + offset, tmp);

      if(strlen(topic->tp_val) != tmp)
	{
	  fprintf(stderr, "El tamaño del valor del tema no coincide con el enviado\n");
	  return NULL;
	}

    }

  return topic;
}
