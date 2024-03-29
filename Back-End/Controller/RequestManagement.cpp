
#include "RequestManagement.h"
#include "String-Operations/base64.h"
#include "String-Operations/hash.h"
#include "Scan-Analyses/MainScan.h"
#include "DownloadManager.h"
#include "String-Operations/randomString.h"
#include "String-Operations/splitString.h"

using std::invalid_argument;
using std::to_string;

void RequestManagement::loginVerification(const CaseInsensitiveMultimap& headers){
    if(headers.empty()){
        throw invalid_argument("Empty header! 'Authorization' is a required header for this request.");
    }
    string verifHeader;
    for(const auto& value : headers){
        if(value.first == "Authorization"){
            verifHeader = value.second;
        }
    }

    if(verifHeader.empty()){
        throw invalid_argument("Missing header! 'Authorization' is a required header for this request.");
    }

    vector<string> verification = splitString(verifHeader, " ");
    //the header needs to be in the format 'Authorization: Bearer token'

    if(!db.checkToken(verification[1])){
        throw invalid_argument("Wrong token! Please authenticate.");
    }
}

string RequestManagement::tokenRequest(const CaseInsensitiveMultimap& parameters){
    string token;

    if(parameters.empty()){
        throw invalid_argument("Empty Parameter! You need a 'login' parameter for this request.");
    }
    string login;
    for(const auto& value : parameters){
        if(value.first == "login"){
            login = value.second;
        }
        else{
            throw invalid_argument("Wrong Parameter! 'login' is the only valid parameter for this request.");
        }
    }
    db.fetchToken(token, login);
    return token;
}
string RequestManagement::authenticateRequest(const CaseInsensitiveMultimap& headers){
    if(headers.empty()){
        throw invalid_argument("Empty header! 'Authorization' is a required header for this request.");
    }
    string authorizeHeader;
    for(const auto& value : headers){
        if(value.first == "Authorization"){
            authorizeHeader = value.second;
        }
    }
    if(authorizeHeader.empty()){
        throw invalid_argument("Missing header! 'Authorization' is a required header for this request.");
    }

    vector<string> authorize = splitString(authorizeHeader, " ");
    authorizeHeader = decode_base64(authorize[1]);
    authorize = splitString(authorizeHeader, ":");
    //the header needs to be in the format 'Authorization: Basic login:password' in base64

    string& login = authorize[0];
    string& password = authorize[1];

    if(!db.checkUser(login, sha512(password))){
        throw invalid_argument("User does not exist!");
    }

    string token = randomString(40);
    db.addToken(token, login);
    return token;
}
string RequestManagement::promotionRequest(const CaseInsensitiveMultimap& headers){
    loginVerification(headers);

    string jsonResponse;
    db.fetchPromotions(jsonResponse);
    return jsonResponse;
}
string RequestManagement::examinationRequest(const CaseInsensitiveMultimap& parameters, const CaseInsensitiveMultimap& headers){
    loginVerification(headers);

    if(parameters.empty()){
        throw invalid_argument("Empty Parameter! You need 'id_promotion' and 'login_teacher' parameters for this request.");
    }
    string id, login;
    for(const auto& value : parameters){
        if(value.first == "id_promotion"){
            id = value.second;
        }
        else if(value.first == "login_teacher"){
            login = value.second;
        }
        else{
            throw invalid_argument("Wrong Parameter! 'id_promotion' and 'login_teacher' are the only valid parameters for this request.");
        }
    }

    string jsonResponse;
    db.fetchExams(jsonResponse, id, login);
    return jsonResponse;
}
string RequestManagement::studentRequest(const CaseInsensitiveMultimap& parameters, const CaseInsensitiveMultimap& headers){
    loginVerification(headers);

    if(parameters.empty()){
        throw invalid_argument("Empty Parameter! You need a 'id_promotion' parameter for this request.");
    }
    string id;
    for(const auto& value : parameters){
        if(value.first == "id_promotion"){
            id = value.second;
        }
        else{
            throw invalid_argument("Wrong Parameter! 'id_promotion' is the only valid parameter for this request.");
        }
    }

    string jsonResponse;
    db.fetchStudents(jsonResponse, id);
    return jsonResponse;
}
string RequestManagement::correctionRequest(const CaseInsensitiveMultimap& parameters, const CaseInsensitiveMultimap& headers, int argc, char** argv){
    loginVerification(headers);

    if(parameters.empty()){
        throw invalid_argument("Empty Parameter! You need 'id_examination' and 'id_student' parameters for this request.");
    }
    string id_examination, id_student;
    for(const auto& value : parameters){
        if(value.first == "id_examination"){
            id_examination = value.second;
        }
        else if(value.first == "id_student"){
            id_student = value.second;
        }
        else{
            throw invalid_argument("Wrong Parameter! 'id_examination' and 'id_student' are the only valid parameters for this request.");
        }
    }

    //Launch Scan-Analyses
    vector<pair <int,int>> answers;
    string stringImage;
    MainScan(argc, argv , stoi(id_examination), stoi(id_student), answers, stringImage);

    //Insert in database
    if (db.NumberResponsesOfStudentsInExamination(id_examination, id_student) == 0){
        db.insertResponses(stoi(id_student), answers);
    }
    //return json
    string jsonResponse;
    db.fetchResponses(stringImage, jsonResponse, id_examination, id_student);
    return jsonResponse;
}

string RequestManagement::correctionPromotionRequest(const CaseInsensitiveMultimap& parameters, const CaseInsensitiveMultimap& headers, int argc, char** argv){
    loginVerification(headers);

    if(parameters.empty()){
        throw invalid_argument("Empty Parameter! You need 'id_examination' and 'id_promotion' parameters for this request.");
    }
    string id_examination, id_promotion;
    for(const auto& value : parameters){
        if(value.first == "id_examination"){
            id_examination = value.second;
        }
        else if(value.first == "id_promotion"){
            id_promotion = value.second;
        }
        else{
            throw invalid_argument("Wrong Parameter! 'id_examination' and 'id_promotion' are the only valid parameters for this request.");
        }
    }

    string jsonResponse;
    vector <int> students = db.getStudentsOfPromotions(id_promotion);
    for(int i=0; i < students.size(); i++){
        vector<pair <int,int>> answers;
        string stringImage;
        MainScan(argc, argv , stoi(id_examination), students[i], answers, stringImage);

        //insert in database
        if (db.NumberResponsesOfStudentsInExamination(id_examination, to_string(students[i])) == 0){
            db.insertResponses(students[i], answers);
        }
    }
    return jsonResponse;
}