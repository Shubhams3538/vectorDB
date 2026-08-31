#include <bits/stdc++.h>
using namespace std;
// taking vector size to be 3 for now 
unordered_map<string,vector<float>>db;


void insert(string str,vector<float>v){
     if(db.find(str) == db.end()){
        db[str] = v;
     }
}

float find_cosine_similarity(vector<float>&a , vector<float>&b){
    float numerator  = a[0] * b[0];
    float num1 = pow(a[0] , 2);
    float num2 = pow(b[0] , 2);
    int n = a.size();
    for(int i = 1;i<n;i++){
        numerator += float(a[i] * b[i]);
        num1 += pow(a[i],2);
        num2 += pow(b[i],2);
    }
    num1 = sqrt(num1);
    num2 = sqrt(num2);
    float deno = num1 * num2;
    return float(numerator / deno);
}

vector<pair<string,float>> search(vector<float>v , int k){
    vector<pair<string,float>>ans;
    // min heap to find top k closest vectors
    priority_queue<pair<float,string>,vector<pair<float,string>>,greater<pair<float,string>>>pq;
    for(auto it : db){
        vector<float>temp = it.second;
        float val = find_cosine_similarity(v,temp);
        if(pq.size() < k){
            pq.push({val,it.first});
        }
        else if(pq.top().first < val){
            pq.pop();
            pq.push({val,it.first});
        }
    }
    while(!pq.empty()){
        ans.push_back({pq.top().second , pq.top().first});
        pq.pop();
    }
    reverse(ans.begin(),ans.end());
    return ans;
}

int size(){
    return db.size();
}
int main(){

   while(true){
    cout<<"Hello welcome to vectorDB!!"<<endl;
    cout<<"1. Search    2.Insert      3.how many vectors   0.Exit"<<endl;
    int val;
    cin>>val;
    if(val == 1){
        int k;
        cout<<"Please input your vector"<<endl;
        vector<float>arr(3);
        for(auto it : arr) cin>>it;
        cout<<"Please enter how many vectors you want to see"<<endl;
        cin>>k;
        vector<pair<string,float>>ans = search(arr,k);
        for(auto it : ans){
            cout<<it.first<<" "<<it.second<<endl;
        }
    }
    else if(val == 2){
        string s;
        cout<<"Plase input the value"<<endl;
        cin>>s;
        cout<<"Please input your vector"<<endl;
        vector<float>arr(3);
        for(auto it : arr) cin>>it;
        insert(s,arr);
        cout<<"Vector inserted !" << endl;

    }

    else if(val == 3){
        cout<<size()<<endl;
    }
    
    else if(val == 0){
        cout<<"Bye!!!"<<endl;
        return 0;
    }
   }
}