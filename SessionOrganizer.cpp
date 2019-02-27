
#include "SessionOrganizer.h"
#include "Util.h"
#include <ctime>

SessionOrganizer::SessionOrganizer ( )
{
    parallelTracks = 0;
    papersInSession = 0;
    sessionsInTrack = 0;
    processingTimeInMinutes = 0;
    tradeoffCoefficient = 1.0;
}

SessionOrganizer::SessionOrganizer ( string filename )
{
    readInInputFile ( filename );
    conference = new Conference ( parallelTracks, sessionsInTrack, papersInSession );
}

void SessionOrganizer::organizePapers ( )
{
    int paperCounter = 0;
    for ( int i = 0; i < conference->getSessionsInTrack ( ); i++ )
    {
        for ( int j = 0; j < conference->getParallelTracks ( ); j++ )
        {
            for ( int k = 0; k < conference->getPapersInSession ( ); k++ )
            {
                conference->setPaper ( j, i, k, paperCounter );
                paperCounter++;
            }
        }
    }
    //applying greedy local search with random walk
    LocalSearch();
    
    
}

void SessionOrganizer::LocalSearch(){

    time_t current_time = time(NULL); //get current time
    int time_to_stop = current_time + processingTimeInMinutes*60-1;

    while(time_to_stop > time(NULL)){

        //get a random paper out of random track's random session
        int pt1 = rand() % conference -> getParallelTracks();
        Track randomtrack1 = conference -> getTrack(pt1);
        int ns1 = rand() % randomtrack1.getNumberOfSessions();
        Session randomsession1 = randomtrack1.getSession(ns1);
        int np1 = rand() % randomsession1.getNumberOfPapers();
        int paper1 = randomsession1.getPaper(np1);
        
        //get another random paper out of random track's random session
        int pt2 = rand() % conference -> getParallelTracks();
        Track randomtrack2 = conference -> getTrack(pt2);
        int ns2 = rand() % randomtrack2.getNumberOfSessions();
        Session randomsession2 = randomtrack2.getSession(ns2);
        int np2 = rand() % randomsession2.getNumberOfPapers();
        int paper2 = randomsession2.getPaper(np2);

        double goodness = optimisedscoreOrganisation( pt1, ns1, np1, pt2, ns2, np2, randomtrack1, randomsession1, randomtrack2, randomsession2);


        conference -> setPaper(pt1,ns1,np1,paper2);
        conference -> setPaper(pt2,ns2,np2,paper1);

        double goodness1 = optimisedscoreOrganisation( pt1, ns1, np1, pt2, ns2, np2, randomtrack1, randomsession1, randomtrack2, randomsession2);
        // cout << goodness1 << endl;
        //swap in case of better state i.e better goodness
        swap(goodness,goodness1,pt1,pt2,ns1,ns2,np1,np2,paper1,paper2);      
    }
}


void SessionOrganizer::swap(double a,double b,int pt1,int pt2,int ns1,int ns2,int np1,int np2,int paper1,int paper2){
	if(a > b){
            conference -> setPaper(pt1,ns1,np1,paper1);
            conference -> setPaper(pt2,ns2,np2,paper2);
        } 

}

void SessionOrganizer::readInInputFile ( string filename )
{
    vector<string> lines;
    string line;
    ifstream myfile ( filename.c_str () );
    if ( myfile.is_open ( ) )
    {
        while ( getline ( myfile, line ) )
        {
            //cout<<"Line read:"<<line<<endl;
            lines.push_back ( line );
        }
        myfile.close ( );
    }
    else
    {
        cout << "Unable to open input file";
        exit ( 0 );
    }

    if ( 6 > lines.size ( ) )
    {
        cout << "Not enough information given, check format of input file";
        exit ( 0 );
    }

    processingTimeInMinutes = atof ( lines[0].c_str () );
    papersInSession = atoi ( lines[1].c_str () );
    parallelTracks = atoi ( lines[2].c_str () );
    sessionsInTrack = atoi ( lines[3].c_str () );
    tradeoffCoefficient = atof ( lines[4].c_str () );

    int n = lines.size ( ) - 5;
    double ** tempDistanceMatrix = new double*[n];
    for ( int i = 0; i < n; ++i )
    {
        tempDistanceMatrix[i] = new double[n];
    }


    for ( int i = 0; i < n; i++ )
    {
        string tempLine = lines[ i + 5 ];
        string elements[n];
        splitString ( tempLine, " ", elements, n );

        for ( int j = 0; j < n; j++ )
        {
            tempDistanceMatrix[i][j] = atof ( elements[j].c_str () );
        }
    }
    distanceMatrix = tempDistanceMatrix;

    int numberOfPapers = n;
    int slots = parallelTracks * papersInSession*sessionsInTrack;
    if ( slots != numberOfPapers )
    {
        cout << "More papers than slots available! slots:" << slots << " num papers:" << numberOfPapers << endl;
        exit ( 0 );
    }
}

double** SessionOrganizer::getDistanceMatrix ( )
{
    return distanceMatrix;
}

void SessionOrganizer::printSessionOrganiser ( char * filename)
{
    conference->printConference ( filename);
}

double SessionOrganizer::scoreOrganization ( )
{
    // Sum of pairwise similarities per session.
    double score1 = 0.0;
    for ( int i = 0; i < conference->getParallelTracks ( ); i++ )
    {
        Track tmpTrack = conference->getTrack ( i );
        for ( int j = 0; j < tmpTrack.getNumberOfSessions ( ); j++ )
        {
            Session tmpSession = tmpTrack.getSession ( j );
            for ( int k = 0; k < tmpSession.getNumberOfPapers ( ); k++ )
            {
                int index1 = tmpSession.getPaper ( k );
                for ( int l = k + 1; l < tmpSession.getNumberOfPapers ( ); l++ )
                {
                    int index2 = tmpSession.getPaper ( l );
                    score1 += 1 - distanceMatrix[index1][index2];
                }
            }
        }
    }

    // Sum of distances for competing papers.
    double score2 = 0.0;
    for ( int i = 0; i < conference->getParallelTracks ( ); i++ )
    {
        Track tmpTrack1 = conference->getTrack ( i );
        for ( int j = 0; j < tmpTrack1.getNumberOfSessions ( ); j++ )
        {
            Session tmpSession1 = tmpTrack1.getSession ( j );
            for ( int k = 0; k < tmpSession1.getNumberOfPapers ( ); k++ )
            {
                int index1 = tmpSession1.getPaper ( k );

                // Get competing papers.
                for ( int l = i + 1; l < conference->getParallelTracks ( ); l++ )
                {
                    Track tmpTrack2 = conference->getTrack ( l );
                    Session tmpSession2 = tmpTrack2.getSession ( j );
                    for ( int m = 0; m < tmpSession2.getNumberOfPapers ( ); m++ )
                    {
                        int index2 = tmpSession2.getPaper ( m );
                        score2 += distanceMatrix[index1][index2];
                    }
                }
            }
        }
    }
    double score = score1 + tradeoffCoefficient*score2;
    return score;
}


double SessionOrganizer::optimisedscoreOrganisation(int pt1,int ns1,int np1,int pt2,int ns2,int np2,Track randomtrack1,Session randomsession1,Track randomtrack2,Session randomsession2)
{
    // Sum of pairwise similarities per session.
    double score = 0.0;
    for ( int k = 0; k < randomsession1.getNumberOfPapers ( ); k++ ){
        int index1 = randomsession1.getPaper ( k );
        for ( int l = k + 1; l < randomsession1.getNumberOfPapers ( ); l++ ){
            int index2 = randomsession1.getPaper ( l );
            score += 1 - distanceMatrix[index1][index2];
        }
    }

    for ( int k = 0; k < randomsession2.getNumberOfPapers ( ); k++ ){
        int index1 = randomsession2.getPaper ( k );
        for ( int l = k + 1; l < randomsession2.getNumberOfPapers ( ); l++ ){
            int index2 = randomsession2.getPaper ( l );
            score += 1 - distanceMatrix[index1][index2];
        }
    } 

    
    // Sum of distances for competing papers.
    double score1 = 0.0;
    int index1 = randomsession1.getPaper(np1);
    for ( int l = 0; l < conference->getParallelTracks ( ); l++ ){
        if(pt1!=l){
            Track tempt = conference->getTrack ( l );
            Session temps = tempt.getSession ( ns1 );
            for ( int m = 0; m < temps.getNumberOfPapers ( ); m++ ){
                int index2 = temps.getPaper ( m );
                score1 += distanceMatrix[index1][index2];
            }
        }
    }

     index1 = randomsession2.getPaper(np2);
    for ( int l = 0; l < conference->getParallelTracks ( ); l++ ){
        if(pt2!=l){
            Track tempt = conference->getTrack ( l );
            Session temps = tempt.getSession ( ns2 );
            for ( int m = 0; m < temps.getNumberOfPapers ( ); m++ ){
                int index2 = temps.getPaper ( m );
                score1 += distanceMatrix[index1][index2];
            }
        }
    }

    return score + tradeoffCoefficient*score1;

  
}
