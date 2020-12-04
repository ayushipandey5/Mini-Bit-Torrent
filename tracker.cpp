#include <bits/stdc++.h>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>

#define BUFF_SIZE 1024
#define LISTEN_PORT 12345			//listen

using namespace std;

string create_userid(char *ptr);
string login(char *ptr);
void error(const char *msg);
vector<string> download(char *ptr);
void upload(char *ptr);
void * acpt(void *ptr);

string create_userid(char *ptr)
{
	string temp(ptr);
	temp += "\n";
	char temp_arr [BUFF_SIZE] ;
	memset ( temp_arr , '\0', BUFF_SIZE);
	strcpy(temp_arr, temp.c_str());

	FILE *fopenfd = fopen("/home/ayushi/Desktop/password.txt", "ab");
	fwrite (temp_arr , strlen (temp_arr), 1, fopenfd);
	fclose(fopenfd);
	return "User added!!!";
}
string login(char *ptr)
{
	string line;
	string usr_pwd(ptr);
	ifstream password_file;
	password_file.open("/home/ayushi/Desktop/password.txt");
	if (!password_file.is_open()) 
	{
	    cout << "Unable to open file tracker.txt"<<endl;
	    exit(1);   // call system to stop
	}
	int flag = 0;
	while ( password_file.good() )
	  {
		// password_file.getline(line, BUFF_SIZE);
	    getline (password_file ,line);
	    if (line.compare(usr_pwd) == 0)	
	    {
	    	flag =1;
	    	break;
	    }	
	   }
	   if (flag == 1)
	   	return "Successfully logged in";
	   else
	   	return "Incorrect user_name/password";
}



void error(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);	
}
vector<string> download(char *ptr)
{ 
		string file(ptr);
	//************** SEARCH IN TRACKER FILE *************************************
		string line;
			ifstream tracker_file;
			tracker_file.open("/home/ayushi/Desktop/tracker.txt");
			if (!tracker_file.is_open()) 
			{
			    cout << "Unable to open file tracker.txt"<<endl;
			    exit(1);   // call system to stop
			}
				vector<string> match;
		if( file.compare("list_files") == 0 )
		{
			while ( tracker_file.good() )
			  {
			    getline (tracker_file ,line);
			    string str = line.substr(0,line.find('|'));
				size_t pos = str.find_last_of('/');	
				match.push_back(str.substr(pos+1));	
			   }
		}
		else
		{
			while ( tracker_file.good() )
			  {
				// tracker_file.getline(line, BUFF_SIZE);
			    getline (tracker_file ,line);
			    string str = line.substr(0,line.find('|'));
				// size_t pos = str.find(file);	//	line.find(text)
				size_t pos = str.find_last_of('/');	
				if (file.compare(str.substr(pos+1)) == 0)
				// if (pos!= string::npos)
			    	match.push_back(line);	
			  }
			    // cout << "line : " << line << endl;
			  // for(int i =0; i<match.size();i++)
			  // 	cout<< match[i]<<endl;
		}
			  return match;
		
		//*******************END OF SEARCH IN TRACKER FILE***************************
}
void upload(char *ptr)
{
	FILE *fopenfd = fopen("/home/ayushi/Desktop/tracker.txt", "ab");
		// FILE *fopenfd = fopen(file_path, "rb");
	fwrite (ptr , strlen (ptr), 1, fopenfd);
	fclose(fopenfd);
}


void * acpt(void *ptr)
{
	int sockfd = *((int *)ptr);
	char command [BUFF_SIZE];
	bzero(command , BUFF_SIZE);
	recv(sockfd , &command , BUFF_SIZE ,0);
	// cout << "size received : " << strlen(command) <<endl;
	
	cout<< "command received :" << command <<endl;
	//************COMMAND SEPERATOR c STYLE****************
	char *s = strdup(command);
	// cout << s<< endl;
	char *words[BUFF_SIZE];
	int size = 0;
	char *word;
	while((word = strsep(&s,"|")) != NULL)
	{ 	
		// cout << size <<"  "<< word <<endl;
		words[size++] = word;
	}
	// for(int i = 0;i<size;i++)
	// 	cout << words[i] <<endl;
	//*********END OF COMAND SEPERATOR C STYLE**************
        if (strcmp(words[0], "download_file") == 0)
        {
        	char *file_name = words[1];
        	vector<string> peers_having_file = download(file_name);
        	cout << "start of download" << endl;
        	int n = peers_having_file.size();
        	send (sockfd , &n ,sizeof(n), 0 );

        	cout << "SENDING " <<endl;
        	// for(int i= 0; i < peers_having_file.size(); i ++) 
     				// cout << peers_having_file[i] <<endl;
        	for(int i =0 ; i < n; i++)
        	{
	        	char temp_arr [BUFF_SIZE] ;
				memset ( temp_arr , '\0', BUFF_SIZE);
	        	strcpy(temp_arr, peers_having_file[i].c_str());
	        	send (sockfd , temp_arr ,sizeof(temp_arr), 0 );
	        	cout << i << "  " << temp_arr << endl;
        	}
        	cout<<"end of download"<< endl;
   //      	pthread_t t2; 
   //      	char *w = word;
			// int xyz = pthread_create(&t2, NULL, download, (void *) w);
        }
        else if (strcmp(words[0], "upload_file") == 0)
        {
        	string str="";
        	for (int i =1;i<size;i++)
        	{
        		string temp(words[i]);
        			str = str + temp +"|";
        	}
        	str[str.length()-1] ='\n';
        	char w[str.length() + 1]; 
        	strcpy(w, str.c_str()); 
        	upload(w);
        }
        else if (strcmp(words[0], "user_pwd") == 0)
        {
        	string str = create_userid(words[1]);
        	char temp_arr [BUFF_SIZE] ;
			memset ( temp_arr , '\0', BUFF_SIZE);
        	strcpy(temp_arr, str.c_str());
        	send (sockfd , temp_arr ,sizeof(temp_arr), 0 );
    	}
        else if (strcmp(words[0], "login") == 0)
        {
        	string str = login(words[1]);
        	char temp_arr [BUFF_SIZE] ;
			memset ( temp_arr , '\0', BUFF_SIZE);
        	strcpy(temp_arr, str.c_str());
        	send (sockfd , temp_arr ,sizeof(temp_arr), 0 );
        }
        else if (strcmp(words[0], "list_files") == 0)
        {
        	char *file_name = "list_files";
        	vector<string> peers_having_file = download(file_name);
        	cout << "start of list_files" << endl;
        	int n = peers_having_file.size() - 1 ;
        	cout<< "n : " << n << endl;
        	send (sockfd , &n ,sizeof(n), 0 );

        	cout << "SENDING " <<endl;
        	// for(int i= 0; i < peers_having_file.size(); i ++) 
     				// cout << peers_having_file[i] <<endl;
        	for(int i =0 ; i < n; i++)
        	{
	        	char temp_arr [BUFF_SIZE] ;
				memset ( temp_arr , '\0', BUFF_SIZE);
	        	strcpy(temp_arr, peers_having_file[i].c_str());
	        	send (sockfd , temp_arr ,sizeof(temp_arr), 0 );
	        	cout << i << "  " << temp_arr << endl;
        	}
        	cout<<"end of list_files"<< endl;
        }


}

int main()
{
	int sockfd, bindfd, acceptfd, readfd;	
	unsigned short portno = LISTEN_PORT;		//atoi(argv[1]);  
    struct sockaddr_in    addr ;
    bzero((char *) &addr , sizeof(addr));  //clears any data the pointer reffers to..
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portno);
    addr.sin_addr.s_addr = INADDR_ANY;  //inet_addr(argv[1]);
    // int opt = 1; 
    int addrlen = sizeof(sockaddr); 
    char buffer[BUFF_SIZE] ; 
	// int socket(int domain, int type, int protocol)
	 sockfd = socket( PF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		error("socket creation failed");



	bindfd = bind(sockfd, (struct sockaddr *) &addr, addrlen);
	if(bindfd < 0)
		error("error in server bind");
	if (listen(sockfd, 3) < 0) 			// 3 = max limit of the client that can connect to the server at a time
		error("error in listening");
	while(1)
	{

		acceptfd = accept(sockfd, (struct sockaddr *)&addr,(socklen_t*) &addrlen);
	    if (acceptfd < 0) 
	    	error("error in accepting");
	    pthread_t t2;
		int *sock1 = &acceptfd;
		int xyz = pthread_create(&t2, NULL, acpt, (void *) sock1);


		const char *file_path = "/home/ayushi/Desktop/tracker.txt" ;
	 //    sendFile(acceptfd, file_path);

	    // cout<<"accepted"<<endl;
	    // close(acceptfd);
	}
	close(sockfd);
    return 0;
}