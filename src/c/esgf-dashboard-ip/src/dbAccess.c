/*
 * dbAccess.c
 *
 *      Author: University of Salento and CMCC 
 */
#include <stdlib.h>
#include <stdio.h>
#include "libpq-fe.h"
#include <string.h>
#include "../include/ping.h"
#include "../include/dbAccess.h"
#include "../include/config.h"
#include "../include/debug.h"

#define CONNECTION_STRING "host=%s port=%d dbname=%s user=%s password=%s"

int retrieve_localhost_metrics()
{
   retrieve_localhost_cpu_metrics();
   return 0;
}

int retrieve_localhost_cpu_metrics()
{
 // retrieving CPU load average metrics
  char esgf_nodetype_filename[256] = { '\0' };
  char query_cpu_metric_insert[2048] = { '\0' };
  char loadavg1[256] = { '\0' };
  char loadavg5[256] = { '\0' };
  char loadavg15[256] = { '\0' };
  int ret_code;

  sprintf (esgf_nodetype_filename, "/proc/loadavg");
  FILE *file = fopen (esgf_nodetype_filename, "r");

  if (file == NULL)		
    return -1;

  if ((fscanf (file, "%s", loadavg1)) == EOF)	
    return -1;			
  if ((fscanf (file, "%s", loadavg5)) == EOF)	
    return -1;			
  if ((fscanf (file, "%s", loadavg15)) == EOF)	
    return -1;			
  pmesg(LOG_DEBUG,__FILE__,__LINE__,"Cpu load average metrics [%s] [%s] [%s]\n", loadavg1, loadavg5,loadavg15);

  snprintf (query_cpu_metric_insert,sizeof (query_cpu_metric_insert),STORE_CPU_METRICS,atof(loadavg1),atof(loadavg5),atof(loadavg15));
	
  if (ret_code = transaction_based_query(query_cpu_metric_insert,START_TRANSACTION_CPU_METRICS,END_TRANSACTION_CPU_METRICS))
      pmesg(LOG_ERROR,__FILE__,__LINE__,"Error storing the cpu load average metrics in the DB! [Code %d]\n",ret_code);

  fclose (file);
  return 0;
}

int transaction_based_query(char *submitted_query, char* open_transaction, char* stop_transaction) 
{
  PGconn *conn;
  PGresult *res;

  //char query_history[2048] = { '\0' };
  char conninfo[1024] = {'\0'};

  /* Connect to database */
  pmesg(LOG_DEBUG,__FILE__,__LINE__,"Transaction based query - START\n");


  snprintf (conninfo, sizeof (conninfo), "host=%s port=%d dbname=%s user=%s password=%s", POSTGRES_HOST, POSTGRES_PORT_NUMBER,POSTGRES_DB_NAME, POSTGRES_USER,POSTGRES_PASSWD);
  conn = PQconnectdb ((const char *) conninfo);

  if (PQstatus(conn) != CONNECTION_OK)
        {
                pmesg(LOG_ERROR,__FILE__,__LINE__,"Connection to database failed: %s\n", PQerrorMessage(conn));
		PQfinish(conn);
		return -1;
        }

	// start transaction

  //pmesg(LOG_DEBUG,__FILE__,__LINE__,"Trying open transaction: [%s]\n",QUERY6);
  //res = PQexec(conn, QUERY6);
  pmesg(LOG_DEBUG,__FILE__,__LINE__,"Trying open transaction: [%s]\n",open_transaction);
  res = PQexec(conn, open_transaction);

  if ((!res) || (PQresultStatus (res) != PGRES_COMMAND_OK))
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__,"Open transaction failed\n");
	        PQclear(res);
		PQfinish(conn);
		return -2;
    	}
  PQclear(res);

  // Query submission 
  // e.g old metrics DELETE Query or pre-computing analytics table 

  //snprintf (query_history,sizeof (query_history),QUERY5,HISTORY_MONTH, HISTORY_DAY);
  //pmesg(LOG_DEBUG,__FILE__,__LINE__,"Removing old metrics: [%s]\n",query_history);
  pmesg(LOG_DEBUG,__FILE__,__LINE__,"Query submission\n");
  //res = PQexec(conn, query_history);
  res = PQexec(conn, submitted_query);

  if ((!res) || (PQresultStatus (res) != PGRES_COMMAND_OK))
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__,"Query submission failed\n");
	        PQclear(res);
		PQfinish(conn);
		return -4;
    	}
  PQclear(res);

       // close transaction
  //res = PQexec(conn, QUERY4);
  //pmesg(LOG_DEBUG,__FILE__,__LINE__,"Trying to close the transaction: [%s]\n",QUERY4);
  res = PQexec(conn, stop_transaction);
  pmesg(LOG_DEBUG,__FILE__,__LINE__,"Trying to close the transaction: [%s]\n",stop_transaction);

  if ((!res) || (PQresultStatus (res) != PGRES_COMMAND_OK))
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__,"Close transaction failed\n");
	        PQclear(res);
		PQfinish(conn);
		return -3;
    	}
  pmesg(LOG_DEBUG,__FILE__,__LINE__,"Transaction based query - END\n");

  PQclear(res);
  PQfinish(conn);

  return 0;
}


struct host * loadHosts(unsigned *numHosts) {
	PGconn *conn;
	PGresult *res;
	int numTuples;

	/* Connect to database */
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"*** Reloading host configuration ***\n");

	char conninfo[1024] = {'\0'};

        snprintf (conninfo, sizeof (conninfo), "host=%s port=%d dbname=%s user=%s password=%s", POSTGRES_HOST, POSTGRES_PORT_NUMBER,POSTGRES_DB_NAME, POSTGRES_USER,POSTGRES_PASSWD);
	conn = PQconnectdb ((const char *) conninfo);

	if (PQstatus(conn) != CONNECTION_OK)
        {
                pmesg(LOG_ERROR,__FILE__,__LINE__,"Connection to database failed: %s\n", PQerrorMessage(conn));
		PQfinish(conn);
		*numHosts = 0;
		struct host* hosts = NULL; 
		return hosts;
        }

	// start transaction
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Trying open transaction\n");
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Query: [%s]\n",QUERY3);
	res = PQexec(conn, QUERY3);

	if ((!res) || (PQresultStatus (res) != PGRES_COMMAND_OK))
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__,"Open transaction failed\n");
	        PQclear(res);
		PQfinish(conn);
		*numHosts = 0;
		struct host* hosts = NULL;
		return hosts;
    	}
	PQclear(res);
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Transaction opened\n");

	// submit SELECT query
	res = PQexec(conn, QUERY1);

	if ((!res) || (PQresultStatus(res) != PGRES_TUPLES_OK))
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__,"SELECT host/services did not work properly\n");
	        PQclear(res);
		PQfinish(conn);
		*numHosts = 0;
		struct host* hosts = NULL;
		return hosts;
    	}

	numTuples = PQntuples(res);
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Number of Host/Services loaded from the DB = %d \n",numTuples);

	struct host* hosts = (struct host *) malloc(sizeof(struct host) * numTuples);
	unsigned t;
	for(t = 0; t < numTuples; t ++) {
		hosts[t].id = atoi(PQgetvalue(res, t, 0));
		strcpy(hosts[t].hostName, PQgetvalue(res, t, 1));
		hosts[t].portNumber = atoi(PQgetvalue(res, t, 2));
	}
	PQclear(res);

	// stop transaction
	res = PQexec(conn, QUERY4);
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Trying to close the transaction\n");
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Query: [%s]\n",QUERY4);

	if ((!res) || (PQresultStatus (res) != PGRES_COMMAND_OK))
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__,"Close transaction failed\n");
	        PQclear(res);
		PQfinish(conn);
		*numHosts = 0;
		free(hosts);
		struct host* hosts = NULL;
		return hosts;
    	}
	PQclear(res);
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Transaction closed\n");
	
    	PQfinish(conn);
	*numHosts = numTuples;
	return hosts;
}

int writeResults(struct host *hosts, const unsigned numHosts) {

	PGconn * conn;
	PGresult *res;
        char conninfo[1024] = {'\0'};
	long int index;

        /* Connect to database */

        snprintf (conninfo, sizeof (conninfo), "host=%s port=%d dbname=%s user=%s password=%s", POSTGRES_HOST, POSTGRES_PORT_NUMBER,POSTGRES_DB_NAME, POSTGRES_USER,POSTGRES_PASSWD);
        conn = PQconnectdb ((const char *) conninfo);

        if (PQstatus(conn) != CONNECTION_OK)
        {
                pmesg(LOG_ERROR,__FILE__,__LINE__,"Connection to database failed: %s", PQerrorMessage(conn));
		PQfinish(conn);
	  	return -1;	
        }

	for(index = 0; index < numHosts; index ++) 
	{
		char insert_query[2048]= {'\0'};

		// Query composition //
		snprintf (insert_query, sizeof (insert_query), "%s VALUES(%d,%d,%d);", QUERY2,hosts[index].status, hosts[index].elapsedTime,hosts[index].id);
		pmesg(LOG_DEBUG,__FILE__,__LINE__,"Service Metrics [%d] -> %s\n",index, insert_query);

		res = PQexec(conn, insert_query);

  		if ((!res) || (PQresultStatus(res) != PGRES_COMMAND_OK))
       		 	pmesg(LOG_ERROR,__FILE__,__LINE__,"Insert SERVICE METRICS query failed\n");

    		PQclear(res);
	}
        PQfinish(conn);
	return 0;
}


int get_single_value(char *submitted_query, long int *metrics) 
{
  PGconn *conn;
  PGresult *res;
  long int numTuples;

  char conninfo[1024] = {'\0'};

  /* Connect to database */
  *metrics=0;
  pmesg(LOG_DEBUG,__FILE__,__LINE__,"Get value - START\n");

  snprintf (conninfo, sizeof (conninfo), "host=%s port=%d dbname=%s user=%s password=%s", POSTGRES_HOST, POSTGRES_PORT_NUMBER,POSTGRES_DB_NAME, POSTGRES_USER,POSTGRES_PASSWD);
  conn = PQconnectdb ((const char *) conninfo);

   if (PQstatus(conn) != CONNECTION_OK)
        {
                pmesg(LOG_ERROR,__FILE__,__LINE__,"Connection to database failed: %s\n", PQerrorMessage(conn));
		PQfinish(conn);
		return -1;
        }

   // Query submission 

   res = PQexec(conn, submitted_query);

   if ((!res) || (PQresultStatus(res) != PGRES_TUPLES_OK))
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__," Get value STOP - Query ERROR \n");
	        PQclear(res);
		PQfinish(conn);
		return -2;
    	}

   numTuples = PQntuples(res);
   pmesg(LOG_DEBUG,__FILE__,__LINE__,"Value [Tuples=%ld] \n",numTuples);
   if (numTuples!=1) 
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__," Get value STOP Too many Tuples ERROR [%ld]\n",numTuples);
	        PQclear(res);
		PQfinish(conn);
		return -3;
    	}

   // setting return metrics	
   *metrics = atol(PQgetvalue(res, 0, 0));

   pmesg(LOG_DEBUG,__FILE__,__LINE__,"Get value - END [value=%ld] \n", *metrics);
   PQclear(res);

   PQfinish(conn);

  return 0;
}


 

int reconciliation_process()
{
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Reconciliation process START\n");
	PGconn *conn;
	PGresult *res;
	long int numTuples;
	long int lastimport_id;
	int ret_code;
	
	lastimport_id=-1;
	/* Connect to database */
	char conninfo[1024] = {'\0'};
	
	if (transaction_based_query(QUERY_DATA_DOWNLOAD_METRICS_DWSTEP1, QUERY8, QUERY4))
		return -1;
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 1 [OK]\n");
	if (transaction_based_query(QUERY_DATA_DOWNLOAD_METRICS_DWSTEP2, QUERY8, QUERY4))
		return -2;
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 2 [OK]\n");
	if (transaction_based_query(QUERY_DATA_DOWNLOAD_METRICS_DWSTEP3, QUERY8, QUERY4))
		return -3;
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 3 [OK]\n");
	if (transaction_based_query(QUERY_DATA_DOWNLOAD_METRICS_DWSTEP4, QUERY8, QUERY4))
		return -4;
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 4 [OK]\n");
	if (transaction_based_query(QUERY_DATA_DOWNLOAD_METRICS_DWSTEP5, QUERY8, QUERY4)) 
		return -5;
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 5 [OK]\n");
	if (transaction_based_query(QUERY_DATA_DOWNLOAD_METRICS_DWSTEP6, QUERY8, QUERY4)) 
		return -6;
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 6 [OK]\n");

	if (ret_code = get_single_value(GET_LAST_IMPORT_ID, &lastimport_id))
		return -7;
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 7.1 [OK]\n",lastimport_id);
	
	if (lastimport_id<0)
		return -8;
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 7.2 [OK] [LastID=%ld]\n",lastimport_id);
	
	if (!lastimport_id) {
		if (transaction_based_query(QUERY_DATA_DOWNLOAD_METRICS_FINALDW_CREATE, QUERY8, QUERY4)) 
			return -9;
 		else 
			pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 7.3 [OK]\n");
	}
	// OPEN CONNECTION  
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 8: START\n");

        snprintf (conninfo, sizeof (conninfo), CONNECTION_STRING, POSTGRES_HOST, POSTGRES_PORT_NUMBER,POSTGRES_DB_NAME, POSTGRES_USER,POSTGRES_PASSWD);
	conn = PQconnectdb ((const char *) conninfo);

	if (PQstatus(conn) != CONNECTION_OK)
        {
                pmesg(LOG_ERROR,__FILE__,__LINE__,"Step 8.1: Connection to database failed: %s\n", PQerrorMessage(conn));
		PQfinish(conn);
		return -10;
        }

	// SELECT START 

	res = PQexec(conn,QUERY_DATA_DOWNLOAD_METRICS_FINALDW_GET_RAW_DATA);

	if ((!res) || (PQresultStatus(res) != PGRES_TUPLES_OK))
    	{
	        pmesg(LOG_ERROR,__FILE__,__LINE__,"Step 8.2: SELECT data from dwstep6 FAILED\n");
	        PQclear(res);
		PQfinish(conn);
		return -11;
    	}

	numTuples = PQntuples(res);
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 8.3: Number of entries from the access_logging table to be processed [%ld] \n",numTuples);

	unsigned t;
	for(t = 0; t < numTuples; t ++) {
		// reconciliation at the tuple level
		if ((t % 100) ==0)
		pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 8.4: Processing entry [%.2f%] \n",(((float) t+1)*100)/((float) numTuples));
		/*hosts[t].id = atoi(PQgetvalue(res, t, 0));
		strcpy(hosts[t].hostName, PQgetvalue(res, t, 1));
		hosts[t].portNumber = atoi(PQgetvalue(res, t, 2));*/
	}
	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 8.4: Processing entry [100%] \n");
	PQclear(res);
	// SELECT END

	// CLOSE CONNECTION  
    	PQfinish(conn);

 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Step 8: END [OK]\n");
 	pmesg(LOG_DEBUG,__FILE__,__LINE__,"Reconciliation process END\n");
 	return 0;
}
