from sklearn import tree
from sklearn.linear_model import LogisticRegression

def process_inputs(filename):
    '''
    Function to process data in filename
    input:
    return:
        data: list of size (n,d) where n is the number of data points each of d dimensions
        labels: list of size n of 0-1 labels
    '''
    pass

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