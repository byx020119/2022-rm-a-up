#include "main.h"
#include "JudgingSystemTask.h"
#include "GunHeartAndBleed.h"
#include "ShootingTask.h"

extGameRobotState_t robotState;
ext_game_status_t   gameState;
extRobotHurt_t      robotHurt;
extPowerHeatData_t  robotPowerHeat;
extShootData_t      robotShootData;
ext_event_data_t    eventState;  //ǰ��ս�Ƿ񱻻������ͺ;������
ext_bullet_remaining_t remainBullet;
ringBuffer_t buffer;

uint32_t ChassisPower_temp;
uint32_t ChassisPower_buffer;
float chassisPowerBuffer = 0;//���ʻ���
float Parameter_Transformation(int32_t data);
//int  Yaw_encoder=0;
float Yaw_encoder=0.0f;
int  Yaw_encoder_s=0;
float test_power=0;

uint8_t Qianshao_state = 0;
uint8_t Shooter_17_Heat1 = 0;
uint8_t Shooter_17_Heat2 = 0; 


/***
������void getGameState(uint8_t *stateData)
���ܣ��Ӳ���ϵͳ��ȡ��Ϸ״̬
��ע��ID��0x0001
      ��11�����ݣ��±�11��12Ϊ��ǰѪ������
***/
void getGameState(uint8_t *stateData)
{
	int tempGameData[12],i,tempTime=0,Game_state;
	for(i=0;i<11;i++)
	{
		tempGameData[i]=stateData[i];
	}
  /***����״̬***/
	Game_state=tempGameData[7];
	gameState.game_type=(Game_state & 0x0F);
	gameState.game_progress=(Game_state & 0xF0)>>4;
	/***ʣ��ʱ��***/
	tempTime=tempGameData[8]|tempGameData[9]<<8;
	gameState.stage_remain_time = Transform_Hex_To_Oct(tempTime,16);
}

/***
������void getEventData(uint8_t *eventData)
���ܣ���ȡ����ǰ��վ��״̬ 0Ϊ�����٣�1Ϊ���
��ע����32�����ݣ���11λ(�±�Ϊ10Ϊ�������ݣ�
***/
void getEventData(uint8_t *eventData)
{
	int tempGameData[32],i=0,Event_state;  //���ùܼ���CRC�м�λ��ֱ�����ĵ��￴�м�λbit
    //��11λ����11λ	 1������ǰ��ս��0������ǰ��վ������ 
	for(i=0;i<32;i++)
	{
		tempGameData[i]=eventData[i];   //�洢ǰ11λ������
	}
	Event_state = tempGameData[8]; //�ڶ�����λ������ʾ��ʽΪ16����
	//Event_stateΪ�м������tempGameData[0]��0x00A5�����������԰˽���Ϊһ����С��׼����7�����ĵ����һ����Ч�İ�λbit�����ڶ���д�Ĵ�СΪ1
	Qianshao_state = (Event_state & 0x04);   
	//������ƴ�СΪ0000 0100����ʮ������Ϊ0x04��λ��õ���11bit������
	//����˳���Ǵӵ�λ����λ(����ʣ��Ѫ����һ��2��С����16λbit��������ôҪ����9��������8λ���ٺ͡�9����λ��)
}

/***
������void getRobotState(uint8_t *stateData)
���ܣ��Ӳ���ϵͳ��ȡ������״̬(��ǰѪ��)
��ע��ID��0x0001
      ��17�����ݣ��±�11��12Ϊ��ǰѪ������
***/
void getRobotState(uint8_t *stateData)
{
	int tempStateData[22],i,tempBlood=0,Robot_numbel;
	for(i=0;i<22;i++)
	{
		tempStateData[i]=stateData[i];
	}
  /***��ȡ��ǰ������ID***/
	Robot_numbel=tempStateData[7];
	robotState.robot_id=Robot_numbel;
	/***��ȡ��ǰѪ��ֵ***/
	tempBlood=tempStateData[9]|tempStateData[10]<<8;
	robotState.remainHP = Transform_Hex_To_Oct(tempBlood,16);
	
	Bleed_Monitor();

}

/***
������void getRobotHurt(uint8_t *hurtData)
���ܣ��Ӳ���ϵͳ��ȡ�˺�����
��ע��ID��0x0002
      ��10�����ݣ��±�7Ϊ�˺�����
***/
void getRobotHurt(uint8_t *hurtData)
{
	int tempHurtData[8],i,hurtInfo;
	for(i=0;i<8;i++)
	{
		tempHurtData[i]=hurtData[i];
	}
	hurtInfo=tempHurtData[7];
	robotHurt.armorType = hurtInfo & 0x0F;      //��������װ��ID
	robotHurt.hurtType  = (hurtInfo & 0xF0)>>4; //�˺�����
	
	Attacked_Monitor();//5/9   ����ǲ���ϵͳ��������֮��Ż�������������������ӳٵĻ���Ӧ�û�Ա�����״̬��Ӱ��

}

/***
������void getRobotPowerHeat(uint8_t *powerHeartData)
���ܣ��Ӳ���ϵͳ��ȡʵʱ������������
��ע��ID��0x0004   50HzƵ�����ڷ���
      ��29�����ݣ��±�23��24Ϊ17mm����ǹ����������
***/
void getRobotPowerHeat(uint8_t *powerHeartData)
{
	int tempPowerHeatData[21],i,temp_17_heat_1,temp_17_heat_2;
	for(i=0;i<21;i++)
	{
	  tempPowerHeatData[i]=powerHeartData[i];
	}
	temp_17_heat_1=tempPowerHeatData[17]|tempPowerHeatData[18]<<8;  //1��
	temp_17_heat_2=tempPowerHeatData[19]|tempPowerHeatData[20]<<8;  //2��
	
	robotPowerHeat.shooter_17_Heat_1 = Transform_Hex_To_Oct(temp_17_heat_1,16);
	robotPowerHeat.shooter_17_Heat_2 = Transform_Hex_To_Oct(temp_17_heat_2,16);
	
  Shooter_17_Heat1 =robotPowerHeat.shooter_17_Heat_1;
  Shooter_17_Heat2 =robotPowerHeat.shooter_17_Heat_2;
}

/***
������void getRobotShootData(uint8_t *shootData)
���ܣ��Ӳ���ϵͳ��ȡʵʱ�����Ϣ
��ע��ID��0x0003
      ��15�����ݣ��±�9��10��11��12Ϊ������������
***/
void getRobotShootData(uint8_t *shootData)
{
	int shoot_17_frequency = 0;
	int tempShootData[15],i;
	GetFloatValueStrcut shoot_17_Speed = SolveFloatValueStr_Init;//4�ֽڸ���������10����С���Ľṹ��
	for(i=0;i<15;i++)
	{
		tempShootData[i] = shootData[i];
	}
//	shoot_17_frequency = tempShootData[8];
	for(i=0;i<4;i++)
	{
		shoot_17_Speed.Temp_Array[i] = tempShootData[i+10];
	}
	robotShootData.bulletType = tempShootData[7];
	if (robotShootData.bulletType==1)
	{
	robotShootData.bulletFreq = shootData[8];
	}
	robotShootData.bulletSpeed = shoot_17_Speed.FloatValueSolve(&shoot_17_Speed);//��ȡ17mm��������
//	BulletSpeed_Transmit((int)robotShootData.bulletSpeed);
	
	/***�Լ�����ǹ������***/
	GunHeart_CalcProcess((int)robotShootData.bulletSpeed);
	
	Bullet_17_real_Speed = robotShootData.bulletSpeed;
	
	//BulletSpeed_Monitor();
	
}

/***
������void getRobotState(uint8_t *stateData)
���ܣ��Ӳ���ϵͳ��ȡ������ʣ�൯��
��ע��ID��0x0208
      �������ݣ��±�Ϊ��ǰ����
***/
void getRemainBulletData(uint8_t *bulletData)
{
	int tempBulletData[12],i,tempBullet=0,remain_Bullet;
	for(i=0;i<11;i++)  //��ȡ��λҲû��
	{
		tempBulletData[i]=bulletData[i];
	}
  /***��ȡ��ǰ������ʣ�൯��***/
	remain_Bullet= tempBulletData[7] | (tempBulletData[8]<<8);//tempBulletData[7]�ǵ���λ���ǵ�һ����Чλ��Ϊ8bit��һ����С
	remainBullet.bullet_remaining_num_17mm=remain_Bullet;  //remainBullet�Ǿ�����֣�bullet_remaining_num_17mm�ǽṹ�����һ������
//	remainBullet.bullet_remaining_num_17mm = Transform_Hex_To_Oct(remain_Bullet,16);	//ת����10���ƣ�ֻ������debug�����Ƿ���ȷ���ѣ����ͨѶ����16����ȡ����
    
}



/*********************************************
������Parameter_Transformation
���ܣ����ֽڶ�������װ��Ϊ������
**********************************************/
float Parameter_Transformation(int32_t data)
{
  int temp1,temp4;
	long temp2;
	float temp3;
	//temp1�ǽ���
	//temp2��β��
	//temp3�������õ���
	//temp4�ǵ���β����ÿһλ
	temp1=((data&0X7F800000)>>23)-127; 
	temp2= data&0X007FFFFF;
	for(int j=0;j<24;j++)
	{
		if(j==0)
		{ 
			temp3=(float)ldexp(1.0,temp1);
		}
		else
		{
		temp4=(temp2&(0x00400000>>(j-1)))>>(23-j);
		temp3=temp3+temp4*(float)ldexp(1.0,temp1-j);
		}
	}
	return temp3;
}



/*************************************
���ܣ�4�ֽڱ�ʾ�ĸ�����תΪ10����С��
��ڲ�����GetFloatValueStrcut���ͽṹ��
���ڲ�����10����С��ֵ
��ע����
**************************************/
float FloatValueSolveFunction(struct GetFloatValueStrcut *date)
{
  int i;
	date->Combination_Array = (date->Temp_Array[0]<<0)|(date->Temp_Array[1]<<8)\
	                                     |(date->Temp_Array[2]<<16)|(date->Temp_Array[3]<<24);//������϶�������ʽ��ʾ�ĸ�����
	date->JieMa = ((date->Combination_Array & 0X7F800000)>>23)-127;
	date->WeiShu = date->Combination_Array & 0X007FFFFF;
	for(i=0;i<24;i++)
	{
		if(i==0) {date->Float_Value = 1*date->ChengFang_Solution(2,date->JieMa);}
		else
		{
			date->WeiShu_EveryBit = (date->WeiShu & (0X00400000>>(i-1)))>>(23-i);
			date->Float_Value = date->Float_Value \
			                              + date->WeiShu_EveryBit*date->ChengFang_Solution(2,date->JieMa-i);
		}
	}
	return date->Float_Value;
}


/*******************
���ܣ��˷����㺯��
��ڲ�����x--����
          n--ָ��
����ֵ���˷�����ֵ
*********************/
float solve_chengfang(float x,int n)         
{
	int i;
	int a;
	float s=1.0;
	if(n>=0) a=n;
	if(n<0) a=-n;
	for(i=1;i<=a;i++)
		s*=x;
	if(n>=0)
	   return s;
	if(n<0)
	  return 1/s;
	return 0;
}

/***
������int Transform_Hex_To_Oct(int data,int len)
���ܣ�16������ת����10������
��ע��data:16���Ƹ�ʽ  len��data��2����λ��
***/
int Transform_Hex_To_Oct(int data,int len)
{
	int a=0x0001,temp=0;
	int ans=0;
	for(int i=0;i<len;i++)
	{
		temp=(data&(a<<i))>>i;
		ans = ans + temp*solve_chengfang(2,i);
	}
	return ans;
}

/***
������void RingBuffer_Write(uint8_t data)
���ܣ�������dataд�뻷�ζ���buffer.ringBuf��
��ע����
***/
void RingBuffer_Write(uint8_t data)
{
	buffer.ringBuf[buffer.tailPosition] = data;     //��β��׷��
	if(++buffer.tailPosition>=BUFFER_MAX)           //β�ڵ�ƫ��
		buffer.tailPosition = 0;                      //����������󳤶ȣ����㣬�γɻ��ζ���
	if(buffer.tailPosition == buffer.headPosition)  //���β���ڵ�׷��ͷ���ڵ㣬���޸�ͷ���ƫ��λ�ö�����������
		if(++buffer.headPosition>=BUFFER_MAX)
			buffer.headPosition = 0;
}

/***
������u8 RingBuffer_Read(uint8_t *pdata)
���ܣ��ӻ��ζ���buffer.ringBuf�ж�ȡ���ݵ���ַpdata��
��ע����
***/
u8 RingBuffer_Read(uint8_t *pdata)
{
	if(buffer.headPosition == buffer.tailPosition)  //���ͷβ�Ӵ���ʾ������Ϊ��
	{
		return 1;  //����1�����λ������ǿյ�
	}
	else
	{
		*pdata = buffer.ringBuf[buffer.headPosition];  //����������ǿ���ȡͷ�ڵ�ֵ��ƫ��ͷ�ڵ�
		if(++buffer.headPosition>=BUFFER_MAX)
			buffer.headPosition = 0;
		return 0;   //����0����ʾ��ȡ���ݳɹ�
	}
	
}
