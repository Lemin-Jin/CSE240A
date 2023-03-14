//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <stdlib.h>

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//gshare global param
uint32_t global_history;  //global history (gshare)
uint32_t mask;            //masking for pc (gshare)
int pht_size;             //size for prediction hashtable (gshare)
uint8_t* pht;             //address for prediction hashtable (gshare)

//lhist global param
uint32_t* lht;            //local history table
uint8_t* lpt;             //local prediction table
int lht_size;             //size of local history tabele
int lpt_size;             //size of local prediction table
uint32_t pc_mask;         //masking for pc (local)
uint32_t lhist_mask;      //masking to extract local history

//tournament global param
uint8_t* tdt;             //tournament decision table
int tdt_size;             //size of tournament decision table




//perceptron global param
#define Budget   (1024*64)
#define numWeights   28
#define threshold   12
#define absMaxWeight 127
#define weightBits    8
#define sizeOfPerceptron ((numWeights+1)*weightBits)
#define numPerceptrons ((int)Budget/sizeOfPerceptron)

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

int32_t perceptronWeightsTable[numPerceptrons][numWeights];
int32_t perceptronBiasTable[numPerceptrons];
uint32_t gHistory;      //global history storage
uint8_t gPrediction;                 //prediction
int prediction;



//initialization function for gshare
void
init_gshare(){

  global_history = 0;               //set global history to all false;

  mask = (1 << ghistoryBits) -1;    //set masking bit 2^(ghistorybit) - 1

  pht_size = 1 << ghistoryBits;     //set prediction table size

  pht = (uint8_t*)malloc(sizeof(uint8_t) * pht_size);  //set the table size of predicion hashtable

  //set the prediction hastable to weakly not taken (01)
  for(int i = 0; i < pht_size; i++){
    pht[i] = WN; 
  }
}

/// @brief initialize local history predictor
void
init_lhist(){

  //create masking for the history bit
  lhist_mask = (1 << lhistoryBits) - 1;

  //initialize local history table
  lht_size = 1 << pcIndexBits;
  lht = (uint32_t*) malloc(sizeof(uint32_t) * lht_size);

  //initialize local prediction table
  lpt_size = 1 << lhistoryBits;
  lpt = (uint8_t*) malloc(sizeof(uint8_t) * lpt_size);

  //set the local history of branch to 0
  for(int i = 0; i < lht_size; i++){
    lht[i] = 0;
  }

  //set the prediction hashtable to weakly not taken
  for(int i = 0; i < lpt_size; i++){
    lpt[i] = WN;
  }

}

/// @brief initialize tournament predictor
void
init_tournament(){
  
  //initialize both local history and gshare as competing predictor
  init_lhist(); 
  init_gshare();

  //the size of tournament decision table has the same size with local history
  tdt_size = lht_size;
  
  //initialize tournamenr decision table
  tdt = (uint8_t*) malloc(sizeof(uint8_t) * tdt_size);

  //tournament decision has 4 stages:
  //00: strongly choose local history
  //01: weakly choose local history
  //10: weakly choose gshare
  //11: strongly choose gshare

  //set the tournament decision table to 01 (Wealy choose local)
  for(int i = 0; i < tdt_size; i++){
    tdt[i] = WN;
  }
}

void
init_perceptron(){
  for(int i = 0; i < numPerceptrons; i++){
    perceptronBiasTable[i] = 0;
    for(int j = 0; j < numWeights; j++){
      perceptronWeightsTable[i][j] = 0;
    }
  }
  printf("Number of perceptrons: %d\n", numPerceptrons);
  printf("Number of weights: %d\n", numWeights);
  printf("Number of weightBits: %d\n", weightBits);
  printf("total memory of perceptron predictor: %d Kbits \n", (numPerceptrons*(numWeights + 1)*(weightBits + 1))/1024);

}

/// @brief prediction function for gshare
/// @param pc program counter
/// @return prediction result of gshare
uint8_t
make_gshare_prediction(uint32_t pc){
  
  uint32_t masked_pc = pc & mask;
  uint32_t masked_ghistory = global_history & mask;

  uint32_t pht_index = masked_pc ^ masked_ghistory;

  //if the precition is 10 or 11, then the branch is taken
  if(pht[pht_index] > WN)
    return TAKEN;
  
  //if the prediction is 01 or 00, then the branch is not taken
  else
    return NOTTAKEN;
}

/// @brief make prediction based on local history
/// @param pc program counter
/// @return prediction result of gshare
uint8_t
make_lhist_prediction(uint32_t pc){

  uint32_t masked_pc = pc & pc_mask;            //mask to get the index for local history table

  int index = lht[masked_pc] & lhist_mask;      //get the content of local history

  int prediction = lpt[index];                  //fetch the prediction (local history as index)

  //if prediction greater than 1 (either 10 / 11), then the branch prediction is taken
  if(prediction > WN)
    return TAKEN;
  //otherwise not taken
  else
    return NOTTAKEN;
}

/// @brief 
/// @param pc 
/// @return 
uint8_t
make_tournament_prediction(uint32_t pc){

  //mask pc to get the index for tournament decision table
  uint32_t index = pc & pc_mask;           

  if(tdt[index] > 1)
    return make_gshare_prediction(pc);
  else
    return make_lhist_prediction(pc);
}


int
make_perceptron_prediction(uint32_t pc){
  int index = pc & (numPerceptrons - 1);
  prediction = perceptronBiasTable[index];
  for (int i = 0; i < numPerceptrons; i++){
    if((gHistory >> i) & 1)
      prediction = prediction + perceptronWeightsTable[index][i];
    else
      prediction = prediction - perceptronWeightsTable[index][i];
  }
  gPrediction = prediction >= 0? TAKEN: NOTTAKEN; 
  return gPrediction;

}


/// @brief update gshare prediction hashtable
/// @param pc program counter
/// @param outcome the outcome of current prediction
void
train_gshare_predictor(uint32_t pc, uint8_t outcome){

  uint32_t masked_pc = pc & mask;                     //mask pc bit
  uint32_t masked_ghistory = global_history & mask;   //mask global history

  uint32_t pht_index = masked_pc ^ masked_ghistory;   //xor global history and pc to get index

  //if stage is not strongly not taken and outcome is false
  if(pht[pht_index] > SN && !outcome)
      pht[pht_index] -= 1;

  //if stage is not strongly taken and outcome is true
  else if(outcome && pht[pht_index] < ST)
    pht[pht_index] += 1;
}


/// @brief update both local history table and prediction table
/// @param pc  prorgam counter
/// @param outcome actual branch taken
void
train_lhist_predictor(uint32_t pc, uint8_t outcome){

  uint32_t masked_pc = pc & pc_mask;              //mask to get the index for local history table
  
  uint32_t local_history = lht[masked_pc];        //get the content of local history

  uint32_t index = local_history & lhist_mask;    //mask to get the index of prediction table

  //if the outcome is true, shift the local history to right and append 1
  if(outcome)
    lht[masked_pc] = (local_history << 1)+ 1;
  //if the outcome is false, shift the local history to right and append 0
  else
    lht[masked_pc] = (local_history << 1);

  //if predicted is not strongly not taken and the outcome is false
  if(lpt[index] > SN && !outcome)
      lpt[index] -= 1;

  //if predicted is not strongly taken and the outcome is true
  else if(outcome && lpt[index] < ST)
    lpt[index] += 1;
}

/// @brief training process of tournament predictor
/// @param pc program counter
/// @param outcome actual branch taken
void
train_tournament_predictor(uint32_t pc, uint8_t outcome){

  //get index for tournament decision table
  uint32_t index = pc & pc_mask;            

  //get gshare prediction
  uint8_t gshare_prediction = make_gshare_prediction(pc);

  //get local history prediction
  uint8_t lhist_prediction = make_lhist_prediction(pc);

  //if gshare is corrent  == 1 else == 0
  uint8_t gshare_correct = gshare_prediction & outcome;

  //if local history make correct prediction == 1 else == 0;
  uint8_t lhist_correct = lhist_prediction & outcome;

  //if lhist and gshare has inconsistent result NOT (0/0), (1/1)
  //a change in prediction need to made
  if(gshare_correct != lhist_correct){
    //if gshare is correct and lhist is not
    if(gshare_correct){
      //modify stage if it is not strongly taken gshare
      if(tdt[index] < 3)
        tdt[index] += 1;
    }
    //if gshare is not correct and lhist is correct
    else{
      //modify stage if it is not strongly taken lhist
      if(tdt[index] > 0)
        tdt[index] -= 1;
    }
  }
}

void
train_perceptron_predictor(uint32_t pc, uint8_t outcome){
  int index = pc & (numPerceptrons - 1);
  //printf("print");
  if ((gPrediction != outcome) || (abs(prediction) < 256))
  {
    
    for (int j = 0; j < numPerceptrons; j++)
    {
      int8_t signMultiplier = ((gHistory >> j)&1) == 1 ? 1: -1; 

      perceptronWeightsTable[index][j] += signMultiplier*(outcome==TAKEN?1:-1);
      perceptronWeightsTable[index][j] = max(min(perceptronWeightsTable[index][j], absMaxWeight), -absMaxWeight-1);
    }
    perceptronBiasTable[index] += (outcome==TAKEN ?1:-1);
  }

  gHistory = (gHistory<<1 | outcome) & ((1<< numWeights) - 1);
}


void
init_predictor()
{

  switch (bpType) {
    case STATIC:
    case GSHARE:
      init_gshare();
      break;
    case TOURNAMENT:
      //init_tournament();
      break;
    case CUSTOM:
      init_perceptron();
      break;
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return make_gshare_prediction(pc);
    case TOURNAMENT:
      //return make_tournament_prediction(pc);
    case CUSTOM:
      return make_perceptron_prediction(pc);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}




// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
   switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      train_gshare_predictor(pc,outcome);
      break;
    case TOURNAMENT:
      //train_tournament_predictor(pc,outcome);
      break;
    case CUSTOM:
      train_perceptron_predictor(pc,outcome);
      break;
    default:
      break;
  }
}