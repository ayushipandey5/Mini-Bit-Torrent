#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
using namespace std;
//what_to_do 	-1=create userid   -2=login  0=upload to tracker  1=download from tracker    5 =list files
//				
#define BUFF_SIZE 1024
#define CHUNK_SIZE 524288
// #define LISTEN_PORT 6666			//listen
// #define CONNECT_PORT 8888			//connect

int LISTEN_PORT;
int login_flag;
string dest_path;
pthread_mutex_t lock1;
struct str_to_struct
{
	string file_path;
	string ip;
	int port;
	string file_parts_to_be_downloaded;
};

void error(const char *msg);
vector<int> create_BitMap(string fp);	
int connect_actual_peer(int cport, string ip);
void list_files_from_tracker(int sockfd);
void login(int sockfd,string user_pwd);
void create_userid(int sockfd,string user_pwd);
vector<vector<string>> extract_details(vector<string> s,char delimiter);
void extract_IP_PORT(vector<string>);
void * acpt(void *ptr);
void connect_peer(int cport,int what_to_do ,string ip, string file_path);
void * menu(void *ptr);
void sendFile(int acceptfd , const char *file_path);
void sendFile( int sockfd , const char *file_path ,vector<int> chunk_nos);
void receiveFile(int sockfd , const char *file_path);
void download_info_from_tracker(int sockfd , string file_path);
void upload_info_to_tracker(int sockfd , string file_path);
void download_file_from_peer(int sockfd, string file_path);
void * downloadFile(void *ptr);
bool file_exists(char * ptr);


bool file_exists(char * ptr)
{
	struct stat buffer;
	return (stat( ptr,&buffer) == 0);
}

void error(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);	
}

void * downloadFile(void *ptr)
{

	struct str_to_struct *temp = (struct str_to_struct *) ptr;
	string file_path1 = temp->file_path;
	string ip1 = temp->ip;
	int port1 = temp->port;
	string file_parts_to_be_downloaded1 = temp->file_parts_to_be_downloaded;
	cout << "file_parts_to_be_downloaded1  " << file_parts_to_be_downloaded1 << endl;
	int sockfd = connect_actual_peer( port1, ip1);
	vector<string> v;
	v.push_back(file_parts_to_be_downloaded1);
	vector<vector<string>> chunk_details = extract_details(v, '|');
	vector<int> chunk_num;
	cout << "chunk_details[0].size()" <<chunk_details[0].size()<< endl;
	cout << "chunk_details[0][40]" << chunk_details[0][40] <<endl;
	for(int i = 0; i < chunk_details[0].size()- 1; i++)
	{
		int temp = stoi(chunk_details[0][i]);
		chunk_num.push_back(temp);
	}
	cout << "outside" << endl;
	pthread_mutex_lock(&lock1);
	cout << "inside" << endl;
		char file_path000 [BUFF_SIZE] ;
		memset ( file_path000 , '\0', BUFF_SIZE);
		strcpy(file_path000, file_path1.c_str());
				string command ="download_file|"  + file_path1 ;
				char temp_arr [BUFF_SIZE] ;
				memset ( temp_arr , '\0', BUFF_SIZE);
	        	strcpy(temp_arr, command.c_str());
	        	send (sockfd , temp_arr ,sizeof(temp_arr), 0 );
	      char buffer[BUFF_SIZE];
	      char d_p [BUFF_SIZE] ;
		  memset ( d_p , '\0', BUFF_SIZE);
    	  strcpy(d_p, dest_path.c_str());

		FILE *fopenfd = fopen(d_p, "r+");

		for( int i =0;i < chunk_num.size() ; i++)
		{	
	       	int n;
	       	int temp = chunk_num[i];
			int chunk_size ;
			send (sockfd , &temp ,sizeof(temp), 0 );
				cout << "asking for chunk num  " << temp << endl;
			recv (sockfd , &chunk_size ,sizeof(chunk_size), 0 );
			cout << "receiving file size " << chunk_size << endl;

			while((chunk_size > 0) && (n = recv(sockfd, buffer, BUFF_SIZE, 0)) > 0)
			{
				cout << "n "<< n <<endl;
				fwrite(buffer, sizeof(char), n, fopenfd);
				memset(buffer, '\0', BUFF_SIZE);
				chunk_size = chunk_size - n;
			} 
		}
		fclose(fopenfd);

	pthread_mutex_unlock(&lock1);
}


void * acpt(void *ptr)
{
	int sockfd = *((int *)ptr);
	// cout<<"acpt" <<endl;

	char file_path [BUFF_SIZE];
	char command_received [BUFF_SIZE];
	char chunk_num [BUFF_SIZE];
	bzero(command_received , BUFF_SIZE);
	bzero(file_path , BUFF_SIZE); 
	bzero(chunk_num , BUFF_SIZE);
		vector<string> v;

	recv( sockfd , &command_received ,  BUFF_SIZE, 0);	// reveive command like get_bitmap
		string cmd_rcvd(command_received);
		v.push_back(cmd_rcvd);
		vector<vector<string>> extracted_details = extract_details(v, '|');
		//extracted_details[0][0] = get_bitmap
		//extracted_details[0][1] = fle_path
	  if (extracted_details[0][0].compare("get_bitmap") == 0)	
	  {
	  	vector<int> bit_map = create_BitMap(extracted_details[0][1]);
	  	string bit_map_str;
	  	for(int i=0; i < bit_map.size() ; i++)
	  		bit_map_str += to_string(bit_map[i]);
		char temp_arr [BUFF_SIZE] ;
		memset ( temp_arr , '\0', BUFF_SIZE);
		strcpy(temp_arr, bit_map_str.c_str());
		send (sockfd , temp_arr ,sizeof(temp_arr), 0 );
	  }
	   else if (extracted_details[0][0].compare("get_file_size") == 0)
	   {
	   	char file_path [BUFF_SIZE] ;
		memset ( file_path , '\0', BUFF_SIZE);
		strcpy(file_path, extracted_details[0][1].c_str());
		FILE *fopenfd = fopen(file_path, "rb");
		fseek ( fopenfd , 0 , SEEK_END);
	  	long long  size = ftell ( fopenfd );	
	  	rewind ( fopenfd );
	  	send (sockfd , &size ,sizeof(size), 0 );
	  	fclose(fopenfd);
	   }
	   else if (extracted_details[0][0].compare("download_file") == 0)
	   {	
		   	char file_path [BUFF_SIZE] ;
			memset ( file_path , '\0', BUFF_SIZE);
			int file_size1 = 0;
			strcpy(file_path, extracted_details[0][1].c_str());
			FILE *fopenfd = fopen(file_path, "rb");
			fseek ( fopenfd , 0 , SEEK_END);
		  	long long  size = ftell ( fopenfd );	
		  	rewind ( fopenfd );
	   			int chunk_num , n;
			recv(sockfd, &chunk_num, sizeof(chunk_num), 0);		//send(sockfd, chunk_numbers,sizeof(chunk_numbers) , 0);

			int no_parts = ceil((double)size / CHUNK_SIZE);
			if(no_parts == chunk_num)
				file_size1 = (size % CHUNK_SIZE);
			else
				file_size1 = CHUNK_SIZE;
			char buffer[BUFF_SIZE];
			send (sockfd , &file_size1 ,sizeof(file_size1), 0 );
				cout << "sending file size " << file_size1 << endl;
				cout << "sending chunk num  " << chunk_num << endl;
	   		fseek(fopenfd, chunk_num*file_size1, SEEK_SET);
	   		while((file_size1 > 0) && (n = fread(buffer, sizeof(char), BUFF_SIZE, fopenfd)) > 0)
			{
				// cout << "sending chunk number " << file_size1 << endl;
				send(sockfd , buffer , n ,0 );
				memset(buffer, '\0', BUFF_SIZE);
				file_size1 = file_size1 - n;
			} 

			fclose(fopenfd);
	   }	
	  // receiveFile(sockfd , file_path ) ;
    // sendFile(acceptfd, file_path);

    // recv( sockfd , Buffer ,   BUFF_SIZE, 0);
    // send (acceptfd , Buffer, n, 0 );

	close(sockfd);
}

void list_files_from_tracker(int sockfd)
{
	 string str = "list_files";

    char temp_arr [BUFF_SIZE] ;
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, str.c_str());
	send (sockfd , temp_arr, BUFF_SIZE, 0 );
	cout << "Sent successfully to tracker for getting list of all files. " << endl;


    // send (sockfd , file_name, strlen(file_name), 0 );
    int n;
    vector<string> peers_having_file ;
    recv (sockfd , &n ,sizeof(n), 0 );
    int recvflag;
    char Buffer [BUFF_SIZE];
    memset ( Buffer , '\0', BUFF_SIZE);
    if (n > 0)
    {
        	cout << "RECEIVING "<< n << " list of files ..." << endl;
        	while( (n > 0)  && ((recvflag = recv (sockfd , Buffer ,BUFF_SIZE, 0 )) > 0) )
        	{
        		// cout << "recvflag "<< recvflag << endl;
        		// cout << n <<"  " << Buffer <<endl;
	        	string temp(Buffer);
	        	peers_having_file.push_back(temp);
	        	n--;
	        	memset ( Buffer , '\0', BUFF_SIZE);

        	}
  
     cout<<"Below are the details received" <<endl;
     for(int i= 0; i < peers_having_file.size(); i ++) 
     	cout << peers_having_file[i] <<endl;
     	// extract_IP_PORT(peers_having_file);
     }
     else
     	cout<< " OOPS, no files to be shares/downloaded!!!" <<endl;
}

void receiveFile(int sockfd , const char *file_path)
{
	FILE *fp = fopen ( file_path , "wb" );
    if(fp == NULL)
    	cout << "not open" << endl;
    int file_size,n;
    char Buffer [BUFF_SIZE];
	bzero(Buffer , BUFF_SIZE); 
	recv(sockfd, &file_size, sizeof(file_size), 0);
    cout<< " We are going to receive a file of size : " << file_size <<" bytes" <<endl;
    while (  file_size > 0 && (n = recv( sockfd , Buffer , BUFF_SIZE, 0)) > 0)
    {
    	cout<< "n " << n <<endl;
        fwrite (Buffer , sizeof (char), n, fp);
        memset ( Buffer , '\0', BUFF_SIZE);
        file_size = file_size - n;
    } 
    fclose ( fp );
}
int connect_actual_peer(int cport, string ip)
{
	int sockfd , connectfd, readfd;
    struct sockaddr_in    addr ;
    bzero((char *) &addr , sizeof(addr));  //clears any data the pointer reffers to..
    addr.sin_family = AF_INET;
    // addr.sin_port = htons(CONNECT_PORT);
    addr.sin_port = htons(cport);
    // addr.sin_addr.s_addr = INADDR_ANY;
    char temp_arr [BUFF_SIZE] ;
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, ip.c_str());
    addr.sin_addr.s_addr=inet_addr(temp_arr);
    int addrlen = sizeof(sockaddr); 
	 sockfd = socket( PF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		error("socket creation failed in connect_peer");
	connectfd = connect(sockfd, (struct sockaddr *)&addr, addrlen ); 
    if(connectfd  <  0)
    	error("error connecting in connect_peer");
    // cout<< "connected to ip : " << ip << " 		port : "<< cport <<endl;
    return sockfd;
}
void connect_peer(int cport,int what_to_do , string ip, string file_path)
{
	int sockfd , connectfd, readfd;
    struct sockaddr_in    addr ;
    bzero((char *) &addr , sizeof(addr));  //clears any data the pointer reffers to..
    addr.sin_family = AF_INET;
    // addr.sin_port = htons(CONNECT_PORT);
    addr.sin_port = htons(cport);
    // addr.sin_addr.s_addr = INADDR_ANY;
    char temp_arr [BUFF_SIZE] ;
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, ip.c_str());
    addr.sin_addr.s_addr=inet_addr(temp_arr);
    int addrlen = sizeof(sockaddr); 
	// int socket(int domain, int type, int protocol)
	 sockfd = socket( PF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		error("socket creation failed in connect_peer");
	connectfd = connect(sockfd, (struct sockaddr *)&addr, addrlen ); 
    if(connectfd  <  0)
    	error("error connecting in connect_peer");
    cout<< "connected " <<endl;
    if (what_to_do == -1)		//create_ueserid
    	create_userid(sockfd,file_path);
    else if (what_to_do == -2)		//login
    	login(sockfd,file_path);
    else if  (what_to_do == 0)
    	upload_info_to_tracker(sockfd,file_path);
    else if (what_to_do == 1)
    {
    	download_info_from_tracker(sockfd,file_path);
    }
    // else if (what_to_do == 2)
    // 	download_file_from_peer(sockfd,file_path);
    else if (what_to_do == 5)
    	list_files_from_tracker(sockfd);
    // else if (what_to_do == 2)
    // 	list_files(sockfd);
    // communicate(sockfd);
    close( sockfd);
        // close(connectfd);

}



void download_file_from_peer(int sockfd, string file_path) //,vector<int> chunk_numb
{
	char temp_arr [BUFF_SIZE] ;
	char chunk_numbers [BUFF_SIZE];
	string chunk_num;
	// for(int i=0;i<chunk_numb.size();i++)
		// chunk_num += to_string(chunk_numb[i]) + "|";
	memset ( chunk_numbers , '\0', BUFF_SIZE);
	strcpy(chunk_numbers, chunk_num.c_str());

	
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, file_path.c_str());
	send (sockfd , temp_arr, sizeof(temp_arr), 0 );	//contains the file_path to be downloaded
	send(sockfd, chunk_numbers,sizeof(chunk_numbers) , 0);	//contains the chunk numbers to be downloaded
}
void upload_info_to_tracker(int sockfd , string file_path)
{
	string str = "upload_file|" + file_path + "|127.0.0.1|" + to_string(LISTEN_PORT);

	char temp_arr [BUFF_SIZE] ;
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, str.c_str());
	send (sockfd , temp_arr, sizeof(temp_arr), 0 );
	cout << "Sent successfully to tracker for upload " << endl;
}

void create_userid(int sockfd,string user_pwd)
{
	string str = "user_pwd|" + user_pwd ;
	char temp_arr [BUFF_SIZE] ;
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, str.c_str());
	send (sockfd , temp_arr, sizeof(temp_arr), 0 );
	// cout << "Sent successfully to tracker for creating user-id " << endl;
	char command [BUFF_SIZE];
	bzero(command , BUFF_SIZE);
	recv(sockfd , &command , BUFF_SIZE ,0);
	cout << command << endl;
}
void login(int sockfd,string user_pwd)
{
	string str = "login|" + user_pwd ;
	char temp_arr [BUFF_SIZE] ;
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, str.c_str());
	send (sockfd , temp_arr, sizeof(temp_arr), 0 );
	// cout << "Sent successfully to tracker for verifying  user-id " << endl;
	char command [BUFF_SIZE];
	bzero(command , BUFF_SIZE);
	recv(sockfd , &command , BUFF_SIZE ,0);
	string temp(command);
	if (temp.compare("Successfully logged in") == 0)
		login_flag = 1; 
	cout << temp <<endl;

}


void download_info_from_tracker(int sockfd , string file_path)
{	
	// char const  *file_name = "download_file|music.mp3|127.0.0.1|8888";
    // const char *file_name = "upload_file|test.mp3|127.0.0.1|1111";
    size_t pos = file_path.find_last_of('/');	
	string file_name = file_path.substr(pos+1);	
    string str = "download_file|" + file_name + "|127.0.0.1|" + to_string(LISTEN_PORT);

    char temp_arr [BUFF_SIZE] ;
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, str.c_str());
	send (sockfd , temp_arr, BUFF_SIZE, 0 );
	cout << "Sent successfully to tracker for getting download info. " << endl;


    // send (sockfd , file_name, strlen(file_name), 0 );
    int n;
    vector<string> peers_having_file ;
    recv (sockfd , &n ,sizeof(n), 0 );
    int recvflag;
    char Buffer [BUFF_SIZE];
    memset ( Buffer , '\0', BUFF_SIZE);
    if (n > 0)
    {
        	cout << "RECEIVING "<< n << " peer details having our files..." << endl;
        	while( (n > 0)  && ((recvflag = recv (sockfd , Buffer ,BUFF_SIZE, 0 )) > 0) )
        	{
        		// cout << "recvflag "<< recvflag << endl;
        		// cout << n <<"  " << Buffer <<endl;
	        	string temp(Buffer);
	        	peers_having_file.push_back(temp);
	        	n--;
	        	memset ( Buffer , '\0', BUFF_SIZE);

        	}
  
     cout<<"Below are the details received" <<endl;
     for(int i= 0; i < peers_having_file.size(); i ++) 
     	cout << peers_having_file[i] <<endl;
     	extract_IP_PORT(peers_having_file);
     }
     else
     	cout<< " OOPS, no peer have our required file!!!" <<endl;
}

vector<vector<string>> extract_details(vector<string> s,char delimiter)
{
	vector<vector<string>> final_result;
	for(int i = 0;i<s.size();i++)
	{
		vector<string> result;
		s[i] += delimiter;
		while(s[i].find(delimiter)!= string::npos)
		{
			result.push_back(s[i].substr(0,s[i].find(delimiter)));
			s[i] = s[i].substr(s[i].find(delimiter)+1,s[i].length());
		}
		final_result.push_back(result);
	}
	return final_result;
}

void extract_IP_PORT(vector <string> v)
{
	vector<vector<string>> extracted_details = extract_details(v, '|');
	// getting bitmap from each peer*****
	// vector<string>bitmap_of_each_peer;
	// for(int i =0; i < v.size(); i++)	
	// {
	// 	int sockfd = connect_actual_peer(stoi(extracted_details[i][2]) , extracted_details[i][1]);
	// 	string str = "get_bitmap|" + extracted_details[i][0] ;
	// 	char temp_arr [BUFF_SIZE] ;
	// 	char BM [BUFF_SIZE];
	// 	memset ( BM , '\0', BUFF_SIZE);
	// 	memset ( temp_arr , '\0', BUFF_SIZE);
	// 	strcpy(temp_arr, str.c_str());
	// 	send (sockfd , temp_arr, sizeof(temp_arr), 0 );
	// 	recv( sockfd , &BM ,  BUFF_SIZE, 0);	// reveive bitmap
	// 	string bitmap(BM);	
	// 	bitmap_of_each_peer.push_back(bitmap);	
	// }


	// end of getting bitmap from each peer*****
	int number_of_peers = v.size();
	long long file_size;
	//***Getting size of the file*******
	int sockfd = connect_actual_peer(stoi(extracted_details[0][2]) , extracted_details[0][1]);
	string str = "get_file_size|" + extracted_details[0][0] ;
		char temp_arr [BUFF_SIZE] ;
		memset ( temp_arr , '\0', BUFF_SIZE);
		strcpy(temp_arr, str.c_str());
		send (sockfd , temp_arr, sizeof(temp_arr), 0 );
		recv( sockfd , &file_size ,  sizeof(file_size), 0);	
	//***End of getting size of file *********
		int no_parts = ceil((double)file_size/CHUNK_SIZE);
		int no_of_chunk_from_peer = no_parts / number_of_peers;
		vector<string> abc;
		int j=0;
		// cout <<"no_of_chunk_from_peer " << no_of_chunk_from_peer << endl;
		string t=""; 
		for( int i = 0 ; i< number_of_peers ; i++)
		{
			// cout <<" i " << i<< endl;
			while( j < no_of_chunk_from_peer*(i+1))
			{
				t += to_string(j) + "|";
				j++;

			}
			abc.push_back(t);
			t="";
		}
		int flag =0;
		for( ;j < no_parts ;j++)
		{
			t +=	to_string(j) + "|";
			flag =1;
		}
		if(flag == 1)
			abc.push_back(t);


			char d_p [BUFF_SIZE] ;
		  memset ( d_p , '\0', BUFF_SIZE);
    	  strcpy(d_p, dest_path.c_str());
    	   // cout << "llli" << endl;
	      if (!file_exists(d_p))
	      {
	      		FILE *f = fopen(d_p, "w");
	      		fseek(f, file_size -1 , SEEK_SET);
	      		fputc('\0',f);
	      		fclose(f);
	      }
	      // cout << "ji" << endl;
	pthread_t threads[number_of_peers];
	for(int i =0; i < number_of_peers; i++)
	{
		struct str_to_struct *connection_details = new str_to_struct();
		connection_details->file_path = extracted_details[i][0];
		connection_details->ip = extracted_details[i][1];
		connection_details->port = stoi(extracted_details[i][2]);
		connection_details->file_parts_to_be_downloaded = abc[i];
		// cout << "ji" << endl;
		if (! pthread_create(&threads[i], NULL, downloadFile, (void *)connection_details ))
		{
			cout<<"Thread created successfully " << endl;
			if(!pthread_detach(threads[i]))
				cout <<"Thread detached successfully" << endl;
		}
	}
	for(int i =0; i < v.size(); i++)
		pthread_join(threads[i],NULL);
}

vector<int> create_BitMap(string fp)
{
	char file_path [BUFF_SIZE] ;
	memset ( file_path , '\0', BUFF_SIZE);
	strcpy(file_path, fp.c_str());
	FILE *fopenfd = fopen(file_path, "rb");
	vector<int> BM;
	fseek ( fopenfd , 0 , SEEK_END);
  	long long  size = ftell ( fopenfd );	
  	rewind ( fopenfd );
  	for(long long i = 0;i <size ; i++)
  	{
  		fseek(fopenfd, i ,SEEK_SET);
  		char c;
  		fscanf(fopenfd, "%c" , &c);
  		if (c == '\0')
  			BM.push_back(0);
  		else
  			BM.push_back(1);
  		i += CHUNK_SIZE;
  	}	
  	return BM;
}



void * menu(void *ptr)
{
	
	int sockfd = *((int *)ptr);
	int cport = 12345;
	while (true)
	{
		string sl;
		cout << "Do you want to Sign-up or login?" << endl;
		cout << "Enter 'create_user' for sign-up and 'login' for login ...." << endl;
		cin >> sl;
		transform(sl.begin(), sl.end(), sl.begin(), ::tolower); 
		if ((sl.compare("create_user") == 0))
		{
			string userid;
			cout << "Enter user id :" << endl;
			cin >> userid;
			char *password = getpass("Enter Password: ");
			string pwd(password);
			string user_pwd = userid + ":" + pwd;
			connect_peer(cport,-1,"127.0.0.1", user_pwd);
		}
		else if(sl.compare("login") == 0)	
		{
			string userid;
			cout << "Enter user id :" << endl;
			cin >> userid;
			char *password = getpass("Enter Password: ");
			string pwd(password);
			string user_pwd = userid + ":" + pwd;
			connect_peer(cport,-2,"127.0.0.1", user_pwd);
		}
		
		{
			while (login_flag)
			{
			string sl;
			cout << "Enter command" << endl;
			cin >> sl;
			transform(sl.begin(), sl.end(), sl.begin(), ::tolower); 
			if(sl.compare("upload_file") == 0)
			{
				cout << "Enter file path to upload : " <<endl;
				string file_path="";								//0 = upload 	 1 = download 	5 = list files
				cin >> file_path;								
				connect_peer(cport,0,"127.0.0.1" , file_path);		// tracker port, 
			}
			else if(sl.compare("download_file") == 0)
			{
				string file_name, file_path,fp;				//0 = upload 	 1 = download 	5 = list files
				cout << "Enter file name to download : " << endl;
				cin >> file_name;		
				cout << "Enter destination path :"	<< endl;
				cin	>>	file_path;
				if (file_path[file_path.length()-1] == '/')
				{
					 dest_path = file_path + file_name;
					 // fp = dest_path + file_name;
				}
				else
				{
					dest_path = file_path + "/" + file_name;
					// fp = dest_path + file_name;
				}

				connect_peer(cport,1,"127.0.0.1", dest_path);		// tracker port, 
			}
			else if( sl.compare("list_files") == 0 )
			{
				string abc = "list_files";
				connect_peer(cport,5,"127.0.0.1", abc);	
			}
			else if(sl.compare("logout") == 0)
				login_flag = 0;
			else 
			{
				cout <<"wrong input... terminating....." << endl;
				continue;
			}
			}
		}
	}
	close(sockfd);	
}

void sendFile(int acceptfd , const char *file_path)
{
	int n = 0;	
	char c;

		FILE *fopenfd = fopen(file_path, "rb");

	fseek ( fopenfd , 0 , SEEK_END);
  	int size = ftell ( fopenfd );	
  	rewind ( fopenfd );

	send ( acceptfd , &size, sizeof(size), 0);
	cout<<"size :"<<size<<endl;
	char Buffer [ BUFF_SIZE] ; 
	bzero(Buffer , BUFF_SIZE);
		

	
	while (  (n = fread( Buffer , sizeof(char) , BUFF_SIZE , fopenfd ))  > 0  && size > 0 )
	{
		send (acceptfd , Buffer, n, 0 );
   	 	memset ( Buffer , '\0', BUFF_SIZE);
		size = size - n ;
	}
	fclose ( fopenfd );
}



int main()
{
	int cport;
	cout<< "Enter LISTEN PORT# : "<< endl;
	cin >> LISTEN_PORT;

	int sockfd, bindfd, readfd;	
	// unsigned short portno = LISTEN_PORT;		//atoi(argv[1]);
	unsigned short portno = LISTEN_PORT;	  
    struct sockaddr_in    addr ;
    bzero((char *) &addr , sizeof(addr));  //clears any data the pointer reffers to..
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portno);
    addr.sin_addr.s_addr = INADDR_ANY;  //inet_addr(argv[1]);
    // int opt = 1; 
    int addrlen = sizeof(sockaddr); 
    char buffer[BUFF_SIZE] ; 
	// int socket(int domain, int type, int protocol)
	printf("local address: %s\n", inet_ntoa( addr.sin_addr));
	// cout << "addr.sin_addr.s_addr  " <<addr.sin_addr.s_addr <<endl;
		// cout << "PF_INET " <<PF_INET<<endl;


	 sockfd = socket( PF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		error("socket creation failed");

	pthread_t t1;
	int *sock = &sockfd;
	int abc = pthread_create(&t1, NULL, menu, (void *) sock);



	bindfd = bind(sockfd, (struct sockaddr *) &addr, addrlen);
	if(bindfd < 0)
		error("error in server bind");
	if (listen(sockfd, 3) < 0) 			// 3 = max limit of the client that can connect to the server at a time
		error("error in listening");
	while(1)
	{

		int acceptfd = accept(sockfd, (struct sockaddr *)&addr,(socklen_t*) &addrlen);
	    if (acceptfd < 0) 
	    	error("error in accepting");
	    pthread_t t2;
		int *sock1 = &acceptfd;
		int xyz = pthread_create(&t2, NULL, acpt, (void *) sock1);


		// const char *file_path = "/home/shubhankar/Desktop/OS/OS_2nd_Assignment/test/diff.txt" ;
	 //    sendFile(acceptfd, file_path);

	    // cout<<"accepted"<<endl;
	    // close(acceptfd);
	}
	close(sockfd);
    return 0;
}