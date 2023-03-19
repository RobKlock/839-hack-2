from sklearn import tree
from sklearn.linear_model import LogisticRegression
import urllib.request
import json,os



def download_data(urlstring='http://3.138.135.239:3000',forced=False):

    if os.path.exists('data.json') and forced==False:
        with open('data.json', 'r') as f:
            raw_data=json.load(f)
    else:
        dataurl=urlstring+'/data'
        with urllib.request.urlopen(dataurl) as url:
            raw_data=json.loads(url.read())
            with open('data.json', 'w') as f:
                json.dump(raw_data, f)
    return raw_data


def process_inputs(raw_data,features=['sonar','bed_sensor']):
    '''
    Function to process data in json dump
    input: json file containing raw sensor data
    return:
        data: list of size (n,d) where n is the number of data points each of d dimensions
        labels: list of size n of 0-1 labels
    '''

    #Clean the inputs
    clean_data=[]
    required_inputs=['labels']+features
    for line in raw_data:
        d=dict(line)
        for feature in required_inputs:
            if feature not in d:
                continue
            clean_data.append(d)

    #data=[sonar, button]
    data=[]
    labels=[]
    for data in clean_data:
        labels.append(data['labels'])
        datum=[]
        for key in features


    return(clean_data)

class DecisionTree:
    def __init__(self):
        self.clf = tree.DecisionTreeClassifier()
    def learn(self,data,labels):
        self.clf = self.clf.fit(data, labels)
    def predict_label(self,data):
        return self.clf.predict(data)
    def predict_probabilities(self,data):
        return self.lf.predict_proba(data)

class LogisticRegression:
    def __init__(self):
        self.clf = LogisticRegression(random_state=0)
    def learn(self,data,labels):
        self.clf=self.clf.fit(data, labels)
    def predict_label(self,data):
        return self.clf.predict(data)
    def predict_probabilities(self,data):
        return self.lf.predict_proba(data)
    
def main():
    print("I do the learning!")

    raw_data=download_data()
    data,labels=process_inputs(raw_data)


    

if __name__ == "__main__":
    main()