#include <stdio.h>  
#include <stdlib.h>
#include <termios.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <string.h>
#include "jni.h"
//#include "JNIHelp.h"
#include <assert.h> 
#include "can.h"
#include <sys/socket.h>
#include <net/if.h>
//#include <cutils/properties.h>
#include <sys/wait.h>

#include <android/log.h>
#include <strings.h>

#define  TAG    "Can_Load_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

/*CAN JNI*/
int canfd=-1;
struct sockaddr_can addr;

void char_strcpy(char *dest, char *src, size_t n)
{
	char i = 0;
	while(i < n)
	{	
		*(dest++) = *(src++);
		i++;
	}
}

static void Java_com_nanochap_test_CanControl_InitCan
  (JNIEnv *env, jobject thiz, jint baudrate)
{

	/* Check arguments */
	switch (baudrate)
	{
		case 5000   :
		case 10000  :
		case 20000  :
		case 50000  :
		case 100000 :
		case 125000 :
			LOGI("Can Bus Speed is %d",baudrate);
		break;
		default:
			LOGI("Can Bus Speed is %d.if it do not work,try 5000~125000",baudrate);
	}

	/* Configure device */
	if(baudrate!=0)
	{
		char str_baudrate[16];

		sprintf(str_baudrate,"%d", baudrate);
		//property_set("net.can.baudrate", str_baudrate); 
		LOGI("str_baudrate is:%s", str_baudrate);
		//property_set("net.can.change", "yes");
	}

	sleep(2);//wait for can0 up
}

static jint Java_com_nanochap_test_CanControl_OpenCan
  (JNIEnv *env, jobject thiz)
{

	struct ifreq ifr;
	int ret;     
	
	/* Opening device */
	canfd = socket(PF_CAN,SOCK_RAW,CAN_RAW);

	if(canfd==-1)
	{
		LOGE("Can Write Without Open"); 
		return   0;
	}

	strcpy((char *)(ifr.ifr_name),"can0");
        ioctl(canfd,SIOCGIFINDEX,&ifr);

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	bind(canfd,(struct sockaddr*)&addr,sizeof(addr));

	LOGI("OpenCan: canfd = %d", canfd);

	return canfd;
}


static jint Java_com_nanochap_test_CanControl_CanWrite
  (JNIEnv *env, jobject thiz, jint canId, jstring data)
{

	int nbytes;
	int num = 0, i = 0;
	struct can_frame frame;

	jboolean iscopy;
	const char *send_data = (*env)->GetStringUTFChars(env, data, &iscopy);	
	
	frame.can_id = canId;
#if 0
	LOGD("CanWrite: canfd = %d, canid = %d, len = %d", canfd, canId, strlen(send_data));

	for(i=0; i<strlen(send_data); i++)
		LOGD("%c", send_data[i]);
#endif
	if(strlen(send_data) > 8)//用于支持当输入的字符大于8时的情况，分次数发送
	{
		num = strlen(send_data) / 8;
		for(i = 0;i < num;i++)
		{
			char_strcpy((char *)frame.data, &send_data[8 * i], 8);
			frame.can_dlc = 8;
			sendto(canfd,&frame,sizeof(struct can_frame),0,(struct sockaddr*)&addr,sizeof(addr));
		}
		memset((char *)frame.data, 0, 8);
		char_strcpy((char *)frame.data, &send_data[8 * i], strlen(send_data) - num * 8);
		frame.can_dlc = strlen(send_data) - num * 8;
		sendto(canfd,&frame,sizeof(struct can_frame),0,(struct sockaddr*)&addr,sizeof(addr));
		nbytes = strlen(send_data);
	}
	else
	{
		char_strcpy((char *)frame.data, send_data, strlen(send_data));
		frame.can_dlc = strlen(send_data);
		sendto(canfd,&frame,sizeof(struct can_frame),0,(struct sockaddr*)&addr,sizeof(addr));
		nbytes = strlen(send_data);
	}

	(*env)->ReleaseStringUTFChars(env, data, send_data);

	LOGD("write nbytes=%d",nbytes);

	return nbytes;
}

static jobject Java_com_nanochap_test_CanControl_CanRead
  (JNIEnv *env, jobject thiz, jobject obj, jint time)
{

	unsigned long nbytes,len;

	struct can_frame frame = {0};
	int k=0;
	jstring   jstr; 
	
	char temp[16];

	fd_set rfds;
	int retval;
	struct timeval tv;
        tv.tv_sec = time;  		
        tv.tv_usec = 0;

	bzero(temp,16);

	if(canfd==-1){
		LOGE("Can Read Without Open");
		frame.can_id=0;
		frame.can_dlc=0;
	}
	else
	{
		FD_ZERO(&rfds);
		FD_SET(canfd, &rfds);
		retval = select(canfd+1 , &rfds, NULL, NULL, &tv);
		if(retval == -1)
		{
			LOGE("Can Read slect error");
			frame.can_dlc=0;
			frame.can_id=0;
		}
		else if(retval)
		{
			nbytes = recvfrom(canfd, &frame, sizeof(struct can_frame), 0, (struct sockaddr *)&addr,&len);
		
			for(k = 0;k < frame.can_dlc;k++)
			{
				//LOGD("%c", frame.data[k]);
				temp[k] = frame.data[k];
			}
			temp[k] = 0;
			
			frame.can_id = frame.can_id - 0x80000000;//读得的id比实际的有个80000000差值，这里需要处理一下
			LOGD("Can Read slect success.");
		}
		else
		{
			frame.can_dlc=0;
			frame.can_id=0;
			//LOGD("Can no data.");
		}

	}
	
		
	jclass objectClass = (*env)->FindClass(env,"com/nanochap/test/CanFrame");
    	jfieldID id = (*env)->GetFieldID(env,objectClass,"can_id","I");
    	jfieldID leng = (*env)->GetFieldID(env,objectClass,"can_dlc","C");
    	jfieldID str = (*env)->GetFieldID(env,objectClass,"data","Ljava/lang/String;");
    
	if(frame.can_dlc) {	
		LOGD("can_id is :0x%x", frame.can_id);
		LOGD("can read nbytes=%d", frame.can_dlc);
		LOGD("can data is:%s", temp);
	}
	
    	(*env)->SetCharField(env, obj, leng, frame.can_dlc);
    	(*env)->SetObjectField(env, obj, str, (*env)->NewStringUTF(env,temp));
    	(*env)->SetIntField(env, obj, id, frame.can_id);
	 
	return   obj;
}

static void Java_com_nanochap_test_CanControl_CloseCan
  (JNIEnv *env, jobject thiz)
{

	if(canfd!=-1)
	{
		close(canfd);
	}

	canfd=-1;

	LOGD("close can0");
}

/*CAN JNI*/
#ifndef NO_REGISTER
static JNINativeMethod gMethods[] = {  
	{"InitCan", "(I)V", (void *)Java_com_nanochap_test_CanControl_InitCan},  
	{"OpenCan", "()I", (void *)Java_com_nanochap_test_CanControl_OpenCan},
	{"CanWrite", "(ILjava/lang/String;)I", (void *)Java_com_nanochap_test_CanControl_CanWrite}, 
	{"CanRead", "(Lcom/nanochap/test/CanFrame;I)Lcom/nanochap/test/CanFrame;", 
						(void *)Java_com_nanochap_test_CanControl_CanRead},
	{"CloseCan", "()V", (void *)Java_com_nanochap_test_CanControl_CloseCan}, 
};
 
static int register_android_CanControl(JNIEnv *env)  
{  
   	jclass clazz;
	static const char* const kClassName =  "com/nanochap/test/CanControl";
	
    	/* look up the class */
    	clazz = (*env)->FindClass(env, kClassName);
    	if(clazz == NULL)
	{
        	LOGE("Can't find class %s\n", kClassName);

        	return -1;
    	}

    	/* register all the methods */
    	if((*env)->RegisterNatives(env,clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) != JNI_OK)
    	{
        	LOGE("Failed registering methods for %s\n", kClassName);
        	
		return -1;
    	}

    	/* fill out the rest of the ID cache */
    	return 0;
}
#endif

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    
#ifndef NO_REGISTER
	JNIEnv *env = NULL;

	LOGD("iTOP-4412 OnLoad CAN JNI start ...");

	if ((*vm)->GetEnv(vm,(void**) &env, JNI_VERSION_1_6) != JNI_OK)
	{
		LOGI("Error GetEnv\n");  
	
		return -1;  
	} 

	assert(env != NULL);  
	if(register_android_CanControl(env) < 0)
	{  
		LOGE("register_android_CAN_JNI error."); 

		return -1;  
	}
#endif
    	/* success -- return valid version number */
	LOGD("iTOP-4412 OnLoad CAN JNI success");

    	return JNI_VERSION_1_4;
}

