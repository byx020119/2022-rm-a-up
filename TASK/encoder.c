#include "main.h"
#include "encoder.h"
#include "CanBusTask.h"


int dtm123[6]; //2022�� ��ʱ������ǹ���� ����Ϊ�ⲿ����

void GetEncoderBias(volatile Encoder *v, CanRxMsg * msg)
{
		v->ecd_bias = (msg->Data[0]<<8)|msg->Data[1];  //�����ʼ������ֵ��Ϊƫ��  
		v->raw_value= v->ecd_bias;
		v->ecd_value = v->ecd_bias;
}

//2310 ��2006ʹ��ͬһ�׽���
void Motor_2310_EncoderProcess(volatile Encoder *v, CanRxMsg * msg)
{
	int i=0;
	int32_t temp_sum = 0;    
	v->last_raw_value = v->raw_value;
	v->raw_value = (msg->Data[0]<<8)|msg->Data[1];
	v->diff = v->raw_value - v->last_raw_value;
	
	if(v->diff < -4096)    //���α������ķ���ֵ���̫�󣬱�ʾȦ�������˸ı�
	{
		v->round_cnt++;
		v->ecd_raw_rate = v->diff + 8192;
	}
	else if(v->diff>4096)
	{
		v->round_cnt--;
		v->ecd_raw_rate = v->diff- 8192;
	}
	else
	{
		v->ecd_raw_rate = v->diff;
	}
	//����diff
	v->rate_buf[v->buf_count++] = v->ecd_raw_rate;
	
	//����õ������ı��������ֵ
	v->ecd_value = v->raw_value + v->round_cnt * 8192;
	
	//����õ��Ƕ�ֵ����Χ���������
	v->ecd_angle = (double)(v->raw_value - v->ecd_bias)*(360/36.0)/8192 + v->round_cnt * 360/36.0;//���ٱ�36:1
	
	if(v->buf_count == RATE_BUF_SIZE)
	{
		v->buf_count = 0;
	}
	//�����ٶ�ƽ��ֵ
	for(i = 0;i < RATE_BUF_SIZE; i++)
	{
		temp_sum += v->rate_buf[i];
	}
	v->filter_rate = (int32_t)(temp_sum/RATE_BUF_SIZE);			
}

void Motor_6623_EncoderProcess(volatile Encoder *v, CanRxMsg * msg)
{
	int i=0;
	int32_t temp_sum = 0;    
	v->last_raw_value = v->raw_value;
	v->raw_value = (msg->Data[0]<<8)|msg->Data[1];
	v->diff = v->raw_value - v->last_raw_value;
	
	if(v->diff < -4096)    //���α������ķ���ֵ���̫�󣬱�ʾȦ�������˸ı�
	{
		v->round_cnt++;
		v->ecd_raw_rate = v->diff + 8192;
	}
	else if(v->diff>4096)
	{
		v->round_cnt--;
		v->ecd_raw_rate = v->diff- 8192;
	}
	else
	{
		v->ecd_raw_rate = v->diff;
	}
	//����diff
	v->rate_buf[v->buf_count++] = v->ecd_raw_rate;
	
	//����õ������ı��������ֵ
	v->ecd_value = v->raw_value + v->round_cnt * 8192;
	
	//����õ��Ƕ�ֵ����Χ���������
	v->ecd_angle = (double)(v->raw_value - v->ecd_bias)*360/8192 + v->round_cnt * 360;   //1:1
	
	if(v->buf_count == RATE_BUF_SIZE)
	{
		v->buf_count = 0;
	}
	//�����ٶ�ƽ��ֵ
	for(i = 0;i < RATE_BUF_SIZE; i++)
	{
		temp_sum += v->rate_buf[i];
	}
	v->filter_rate = (int32_t)(temp_sum/RATE_BUF_SIZE);			
}

//���õ��Ǿ���ʽ������,����ʽ�Ƕ�-���ֱ�����,13λ���̣�
//�ٶ������ڶ�ȡ�����ڣ���ת�Ƕ�С��180�㣻
//ƫ��ֵС��-4096��˵��˳ʱ��ͨ����㣬Ȧ��+1;ƫ��ֵ����4096��˵����ʱ��ͨ����㣬Ȧ��-1��
//˳ʱ��ͨ����㣬��ʾ��������ʼ����һȦ����ʱ��ͨ����㣬��ʾ�������˻���һȦ��
//����������ֵ������0Ϊ˳ʱ�룻С��0Ϊ��ʱ�룻
//2^13=8192,�Ƕȷֱ���Ϊ360/(2^13)=0.0439�㣻
//��ȡ����T���㣺2��*rmp*T/60<��,�ã�T<30/rmp (s)   ,����rmpΪ������ת�٣���λȦ/�֣�
//���̣�˳ʱ��Ϊ����ֵ����ķ�����ʱ��Ϊ����ֵ��С�ķ���
//
//            0
//
//     
//   6144           2048   
//
//
//          4096

void Motor_6020_EncoderProcess(volatile Encoder *v, CanRxMsg * msg)
{
	int i=0;
	int32_t temp_sum = 0;
  int16_t torque_current6623 = 0;
  int16_t motor_speed = 0;	
	v->last_raw_value = v->raw_value;
	v->raw_value = (msg->Data[0]<<8)|msg->Data[1];
	v->diff = v->raw_value - v->last_raw_value;
	
	if(v->diff < -4096)    //���α������ķ���ֵ���̫�󣬱�ʾȦ�������˸ı�
	{
		v->round_cnt++;
		v->ecd_raw_rate = v->diff + 8192;
	}
	else if(v->diff>4096)
	{
		v->round_cnt--;
		v->ecd_raw_rate = v->diff- 8192;
	}
	else
	{
		v->ecd_raw_rate = v->diff;
	}
	//����diff
	v->rate_buf[v->buf_count++] = v->ecd_raw_rate;
	
	//����õ������ı��������ֵ
	v->ecd_value = v->raw_value + v->round_cnt * 8192;
	
	//����õ��Ƕ�ֵ����Χ���������
	v->ecd_angle = (double)(v->raw_value - v->ecd_bias)*360/8192 + v->round_cnt * 360;//+ v->round_cnt * 360;
	v->ecd_xtl_angle = (double)(v->raw_value - v->ecd_bias)*(360)/8192;	
	
	if(v->buf_count == RATE_BUF_SIZE)
	{
		v->buf_count = 0;
	}
	//�����ٶ�ƽ��ֵ
	for(i = 0;i < RATE_BUF_SIZE; i++)
	{
		temp_sum += v->rate_buf[i];
	}
	motor_speed = (msg->Data[2]<<8)|msg->Data[3];
	v->filter_rate = motor_speed *360/60;	
	torque_current6623= (msg->Data[4]<<8)|msg->Data[5];
	v->real_torque_current = torque_current6623;
}


void Motor_3508_EncoderProcess(volatile Encoder *v, CanRxMsg * msg)
{
	int i=0;
	int32_t temp_sum = 0;
  int16_t temp_filter=0;	
	int16_t torque_current = 0;
	v->last_raw_value = v->raw_value;
	v->raw_value = (msg->Data[0]<<8)|msg->Data[1];
	v->diff = v->raw_value - v->last_raw_value;
	
	if(v->diff < -4096)    //���α������ķ���ֵ���̫�󣬱�ʾȦ�������˸ı�
	{
		v->round_cnt++;
		v->ecd_raw_rate = v->diff + 8192;
	}
	else if(v->diff>4096)
	{
		v->round_cnt--;
		v->ecd_raw_rate = v->diff- 8192;
	}
	else
	{
		v->ecd_raw_rate = v->diff;
	}
	//����diff
	v->rate_buf[v->buf_count++] = v->ecd_raw_rate;
	
	//����õ������ı��������ֵ
	v->ecd_value = v->raw_value + v->round_cnt * 8192;
	
	//����õ��Ƕ�ֵ����Χ���������
	v->ecd_angle = (double)(v->raw_value - v->ecd_bias)*(360/19.0)/8192 + v->round_cnt * 360/19.0;//���ٱ�36:1
	
	if(v->buf_count == RATE_BUF_SIZE)
	{
		v->buf_count = 0;
	}
	//�����ٶ�ƽ��ֵ
	for(i = 0;i < RATE_BUF_SIZE; i++)
	{
		temp_sum += v->rate_buf[i];
	}
	//v->filter_rate = (int32_t)(temp_sum/RATE_BUF_SIZE);	
	temp_filter = ((msg->Data[2]<<8)|msg->Data[3]);
	v->filter_rate = temp_filter/19.;
	torque_current=((msg->Data[4]<<8)|msg->Data[5]);
	v->real_torque_current = torque_current;
}



//2022�� ���ͨ�� Down to Up����
//���ݾ���������ʱδ��

void Down_to_Up_Flag(CanRxMsg * msg)
{
	dtm123[0]=msg->Data[0];
	dtm123[1]=msg->Data[1];
	dtm123[2]=msg->Data[2];
	dtm123[3]=msg->Data[3];
	dtm123[4]=msg->Data[4];
	dtm123[5]=msg->Data[5];
}
