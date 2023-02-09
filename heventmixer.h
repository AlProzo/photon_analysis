#ifndef HEVENTMIXER_H
#define HEVENTMIXER_H

////////////////////////////////////////////////////////////////////////////////
//
// This is a set of classes providing a container for mixed events. It allows
// to store buffers of defined length (corresponding to events of various
// physical characteristics, like centrality. target segment, etc.) with lists
// of particles of different types (PIDs) present in these events. Empty events,
// i.e. those where no particle of any requested type were present are not
// stord. Particles can be represented by any type of objects: TLorentzVector,
// HParticleCand, HGeantKine, PParticle. Mixing of other classes or even scalar
// numbers is also possible.
//
// HEventMixr is an abstract tmplate class that defines the mechanism of event
// mixing. Since the meaningful building of pairs of particles from different
// events depends on the class used to represent particles, there are a few
// concrete classes:
// HParticleEventMixer - to mix HParticleCand objects, mixed pairs are 
//                       represented as HParticlePair
//
// HGenericEventMixer  - capable to mix objects of any type T, mixed pairs
//                       are represented as std::pair<T*, T* >
//
// The only requirement to use these classes is to include this header file.
//
// Usage exapmples:
//
// 1. HParticleEventMixer
//
//    // In the beginning of the program
//    HParticleEventMixer eventmixer;
//    eventmixer.setPIDs(2,3,1);  // which PIDs and MotherID are stored 
//                                // in HParticlePair
//    eventmixer.setBuffSize(80); 
//    eventmixer.setEventClassifier(eventClassifier);
//    // eventClassifier is a user-defined function that accepts no arguments
//    // and return an integer number, that is different for each event types
//    // that should be mixed separately
//
//    // Inside the event loop:
//    vector<HParticleCand *> vep;
//    vector<HParticleCand *> vem;
//    // Now the vectors have to be filled with selected particles. Note that
//    // copies of HParticlesCand objects are created here, because, the 
//    // original objects are not available anymore after leaving moving to the
//    // next event
//    for(Int_t j = 0; j < size; j ++){
//        cand1 = HCategoryManager::getObject(cand1,candCat,j);
//        if (leptonFlag[j]) {
//            if (cand1->getCharge() == -1) {
//                vem.push_back(new HParticleCand(*cand1));
//            }
//            if (cand1->getCharge() == 1) {
//                vep.push_back(new HParticleCand(*cand1));
//            }
//        }
//    }
//    eventmixer.nextEvent();
//    eventmixer.addVector(vep,2);
//    eventmixer.addVector(vem,3);
//    vector<HParticlePair>& pairsVec = eventmixer.getMixedVector();
//    // Now one can loop over pairsVec and do interesting work e.g. fill 
//    // physics spectra. After this, the copies of particle candidates, created
//    // above must be destroyed, when the events, that contains them goes out
//    // of the buffer:
//    vector <HParticleCand *>* toDel = eventmixer.getParticlesToDelete();
//    for (unsigned int ii = 0; ii < toDel->size(); ++ii) {
//        delete toDel->at(ii);
//    }
//    toDel->clear();
//    delete toDel;
//
// 2. HGenericEventMixer (example with HGeantKine)
//    
//    // In the beginning of the program
//    HGenericEventMixer<HGeantKine> eventmixer;
//    eventmixer.setPIDs(2,3,1);  // in this case the numbers are arbitrary,
//                                // but must be different
//    eventmixer.setBuffSize(80);
//    eventmixer.setEventClassifier(eventClassifier);   
//    // eventClassifier is a user-defined function that accepts no arguments
//    // and return an integer number, that is different for each event types
//    // that should be mixed separately
//
//    // Inside the event loop:
//    vector<HGeantKine *> vep;
//    vector<HGeantKine *> vem;
//    // Now the vectors have to be filled with selected particles. Note that
//    // copies of HGeantKine objects are created here, because, the 
//    // original objects are not available anymore after leaving moving to the
//    // next event
//    for(Int_t j = 0; j < size; j ++){
//        kine1 = HCategoryManager::getObject(kine1,kineCat,j);
//        if (kine1->getTotalMomentum() > 100 && kine1->getTotalMomentum() < 1000) {
//            if (kine1->getID() == 3) {
//               vem.push_back(new HGeantKine(*kine1));
//            }
//            if (kine1->getID() == 2) {
//                vep.push_back(new HGeantKine(*kine1));
//            }
//        }
//    }
//    eventmixer.nextEvent();
//    eventmixer.addVector(vep,2);
//    eventmixer.addVector(vem,3);
//    vector<pair<HGeantKine *, HGeantKine* > >& pairsVec = eventmixer.getMixedVector();
//    // Now one can loop over pairsVec and do interesting work e.g. fill 
//    // physics spectra. 
//
//    size = pairsVec.size();
//
//    for (Int_t j = 0; j < size; j ++) {
//        pair<HGeantKine*,HGeantKine*>& pair = pairsVec[j];
//        kine1 = pair.first;
//        kine2 = pair.second;
//        TLorentzVector vec1, vec2;
//        HParticleTool::getTLorentzVector(kine1,vec1,kine1->getID());
//        HParticleTool::getTLorentzVector(kine2,vec2,kine2->getID());
//        dilep = vec1 + vec2;
//    }
//
//    // After this, the copies of particle candidates, created
//    // above must be destroyed, when the events, that contains them goes out
//    // of the buffer:
//    vector <HGeantKine *>* toDel = eventmixer.getParticlesToDelete();
//    for (unsigned int ii = 0; ii < toDel->size(); ++ii) {
//        delete toDel->at(ii);
//    }
//    toDel->clear();
//    delete toDel;
//
////////////////////////////////////////////////////////////////////////////////
//
// It's possible, but not mandatory, to have different buffer sizes for
// different event classes. If you want this option, just use:
//    eventmixer.setBuffSize(size,eventClass);
// The default value of eventClass is 0. If no size is explicitely settled for
// some event class, then also size for class 0 is taken into account.
//
////////////////////////////////////////////////////////////////////////////////

//Standard headers
#include <map>
#include <vector>
#include <deque>

//ROOT headers

//Hydra headers
#include "heventheader.h"
#include "hparticlecand.h"
#include "hparticletracksorter.h"
#include "hparticlepairmaker.h"
#include "hparticleevtinfo.h"
#include "hloop.h"

int defaultEventClassifier() {
    return 0;
}

//Generic framework class used for event mixing
template <class T, class R>
class HEventMixer {
    public:
        //Aliases for comppex types used in the code
        typedef std::vector<T> ParticlesList;
        typedef std::map<int, ParticlesList> EventToMix;
        typedef std::deque<EventToMix> EventQueue;
        typedef std::map<int, EventQueue> DataStructure;

        typedef int (*EventClassifier) ();

        HEventMixer();
        ~HEventMixer();

        void setPIDs(Int_t pid1,Int_t pid2,Int_t motherpid) {
            fPID1      = pid1;
            fPID2      = pid2;
            fMotherPID = motherpid;
        }

        void setEventClassifier(EventClassifier classifier) {
            eventClassifier = classifier;
        }

        // Sets the size of the buffer and minimum number of events that have to be collected before they start to be mixed for the 
        // events class eventClass to the value n. If no event class is specified, the values are set for the class number 0
        void setBuffSize(UInt_t n,UInt_t eventclass = 0) {
            buffsize[eventclass] = n;
            minbuffsize[eventClass] = buffsize[eventClass];
        }
        // Specifies if the mixing should start only after the buffer reached the requested size, 
        // CAUTION: This function should be called after all setBuffSize calls, Otherwise, the result of setWaitForBuffer
        // will be overwritten.
        void setWaitForBuffer(Bool_t b) { 
            std::map<Int_t, UInt_t>::iterator it = buffsize.begin();
            for ( ; it != buffsize.end(); ++it) {
                minbuffsize[it->first] = b ? it->second : 1;
            }
        }

        void nextEvent();
	void nextEvent(const int eventClassNum);
        void addVector(vector<T> v, int id);
        vector<R>& getMixedVector();
        vector<R>& getMixedVector(Int_t pid1, Int_t pid2, Int_t motherPid);
        Int_t pokeMixedVector ();
        std::vector<T * >* getObjectsToDelete();

        vector<T>& getReferenceVector() { return freference;    }
        vector<T>& getOthersVector() { return fothers;       }
        void printDataStructure();

        void setUseLeptons (Bool_t use) { fuse_kIsLepton = use;}
        void setVertexCase(Particle::eVertex vertexCase) { fVertexCase =  vertexCase; };
        void setVertex(HGeomVector& v)  { fVertex = v; fVertexCase = kVertexUser;}
        void setVertex(HVertex& v)      { fVertex = v.getPos(); fVertexCase = kVertexUser;}

        void countEventsInClasses(HLoop *loop);
        int currentEventClass() { return eventClass; }
        double eventWeight() { return 1./nEvtOfClass[eventClass]; }
        double eventWeight(int evtclass) { return 1./nEvtOfClass[evtclass]; }
        long getNEvtOfClass(int eventClass) { return nEvtOfClass[eventClass]; }
        long getNEvtOfCurrentClass() { return nEvtOfClass[eventClass]; }
        bool isEmptyEvent() { return emptyEvent; }


    protected:
        vector<T> freference;
        vector<T> fothers;
        vector<R> fpairs;
        map<HParticleCand*,vector<HParticlePair*> > mCandtoPair;

        EventToMix eventToRemove;

        Int_t fPID1;
        Int_t fPID2;
        Int_t fMotherPID;

        Bool_t      emptyEvent;
        Bool_t      fuse_kIsLepton;
        Int_t       fVertexCase;
        HGeomVector fVertex;

        void clearVectors();
        void selectPID(HParticleCand* cand1,Int_t& pid1,Bool_t warn=kTRUE);
        virtual R buildPair(T, T) = 0;
        DataStructure dataStructure;
        EventClassifier eventClassifier;
        std::map<int, long> nEvtOfClass;

        std::map<Int_t, UInt_t> buffsize;
        std::map<Int_t, UInt_t> minbuffsize;
        int eventClass;
};

template <class T, class R>
HEventMixer<T,R>::HEventMixer()
{
    eventClassifier = defaultEventClassifier;
    buffsize[0] = 20; //default value
    setPIDs(HPhysicsConstants::pid("e+"),HPhysicsConstants::pid("e-"),HPhysicsConstants::pid("dilepton"));
    fVertexCase = kVertexParticle;
    fuse_kIsLepton = kTRUE;

    fothers   .resize(100);
    freference.resize(100);
    fpairs    .resize(100);
}

template <class T, class R>
HEventMixer<T,R>::~HEventMixer()
{
    clearVectors();
}


template <class T, class R>
void HEventMixer<T,R>::countEventsInClasses(HLoop *loop) {
    //Pre-loop to calculate etm class weigths
    Int_t nevts = loop->getEntries();
    for (int ievt = 0; ievt < nevts; ++ievt) {
        loop->nextEvent(ievt);
        eventClass = eventClassifier();
        if (nEvtOfClass.count(eventClass) == 0) {
            nEvtOfClass[eventClass] = 1;
        }
        else {
            nEvtOfClass[eventClass]++;
        }
    }
}


template <class T, class R>
void HEventMixer<T,R>::nextEvent() {
    emptyEvent = true;
    clearVectors();

    //Calling of the etm classifier provided as a function pointer by the class client
    eventClass = eventClassifier();

    bool lastEventEmpty = kTRUE;
    if (dataStructure[eventClass].size() > 0) {
        EventToMix lastEtmp = dataStructure[eventClass].back();
        typename EventToMix::iterator lastEtmpIt = lastEtmp.begin();
        int k = 0;
        for ( ; lastEtmpIt != lastEtmp.end(); ++k, ++lastEtmpIt) {
            ParticlesList &list = lastEtmpIt->second;
            if (list.size() > 0) {
                lastEventEmpty = kFALSE;
                break;
            }
        }
    }
    else {
        lastEventEmpty = kFALSE; // if there is no previous event, it cannot be an empty event
    }
    if (!lastEventEmpty) {
        EventToMix etmp;
        dataStructure[eventClass].push_back(etmp);
    }
}


template <class T, class R>
void HEventMixer<T,R>::nextEvent(int eventClassNum) {
    emptyEvent = true;
    clearVectors();

    //Calling of the etm classifier provided as a function pointer by the class client
    eventClass = eventClassNum;

    bool lastEventEmpty = kTRUE;
    if (dataStructure[eventClass].size() > 0) {
        EventToMix lastEtmp = dataStructure[eventClass].back();
        typename EventToMix::iterator lastEtmpIt = lastEtmp.begin();
        int k = 0;
        for ( ; lastEtmpIt != lastEtmp.end(); ++k, ++lastEtmpIt) {
            ParticlesList &list = lastEtmpIt->second;
            if (list.size() > 0) {
                lastEventEmpty = kFALSE;
                break;
            }
        }
    }
    else {
        lastEventEmpty = kFALSE; // if there is no previous event, it cannot be an empty event
    }
    if (!lastEventEmpty) {
        EventToMix etmp;
        dataStructure[eventClass].push_back(etmp);
    }
}

template <class T, class R>
void HEventMixer<T,R>::clearVectors() {
    freference.clear();
    fothers.clear();
    fpairs.clear();
    mCandtoPair.clear();
}

template <class T, class R>
void HEventMixer<T,R>::addVector(vector<T> v, int id) {
    dataStructure[eventClass].back()[id] = v;
    if (v.size() > 0) emptyEvent = false;
}

template <class T, class R>
Int_t  HEventMixer<T,R>::pokeMixedVector () {
    Int_t buffsize_key = 0;
    if (buffsize.find(eventClass) != buffsize.end()) {
        buffsize_key = eventClass;
    }
    Int_t n_mixes = 0;
    if (dataStructure[eventClass].size() > minbuffsize[buffsize_key] && !emptyEvent) {
        n_mixes = dataStructure[eventClass].size() - 1;
        if (dataStructure[eventClass].size() > buffsize[buffsize_key]) {
            eventToRemove = dataStructure[eventClass].front();
            dataStructure[eventClass].pop_front();
        }
    }
    return n_mixes;
}

template <class T, class R>
vector<R>& HEventMixer<T,R>::getMixedVector (Int_t pid1, Int_t pid2, Int_t motherPid) {
    setPIDs(pid1,pid2,motherPid);
    return getMixedVector();
}

#warning some lines commented out

template <class T, class R>
vector<R>& HEventMixer<T,R>::getMixedVector () {
    Int_t buffsize_key = 0;
    if (buffsize.find(eventClass) != buffsize.end()) {
        buffsize_key = eventClass;
    }
    if (dataStructure[eventClass].size() > minbuffsize[buffsize_key] && !emptyEvent) {
        T obj1, obj2;
        EventToMix &etm = dataStructure[eventClass].back();
        copy(etm[fPID1].begin(), etm[fPID1].end(), std::back_inserter(freference));
        //copy(etm[fPID2].begin(), etm[fPID2].end(), std::back_inserter(freference));
        //1. Iterate over all previous etms in the queue
        for (unsigned int i = 0; i < dataStructure[eventClass].size() - 1; ++i) {
            //2. Iterate over all ordered pairs, which have a mixing function as a second element
            EventToMix otherEtm = dataStructure[eventClass][i];
            //copy(otherEtm[fPID1].begin(), otherEtm[fPID1].end(), std::back_inserter(fothers));
            copy(otherEtm[fPID2].begin(), otherEtm[fPID2].end(), std::back_inserter(fothers));
        }

        if (dataStructure[eventClass].size() > buffsize[buffsize_key]) {
            eventToRemove = dataStructure[eventClass].front();
            dataStructure[eventClass].pop_front();
        }
        for(UInt_t i = 0 ; i < freference.size(); i++) {
            obj1 = freference[i];

            for(UInt_t j = 0 ; j < fothers.size(); j++) {
                obj2 = fothers[j];

                fpairs.push_back(buildPair(obj1,obj2));
            } // end loop others
        } // end loop reference
        //-------------------------------------------------------
    }
    return fpairs;
}

template <class T, class R>
vector<T * >* HEventMixer<T,R>::getObjectsToDelete() {
    vector<T *>* particlesToDelete = new vector<T *>();
    typename EventToMix::iterator evtIt = eventToRemove.begin();
    for ( ; evtIt != eventToRemove.end(); ++evtIt) {
        ParticlesList particles = evtIt->second;
        typename ParticlesList::iterator particlesIt = particles.begin();
        for ( ; particlesIt != particles.end(); ++particlesIt) {
            particlesToDelete->push_back(*particlesIt);
        }
    }
    eventToRemove.clear();
    return particlesToDelete;
}

template <class T, class R>
void HEventMixer<T,R>::printDataStructure() {
    cout << "dataStructure.size() = " << dataStructure.size() << endl;
    typename DataStructure::iterator dsIt = dataStructure.begin();
    for ( ; dsIt != dataStructure.end(); ++dsIt) {
        cout << "\tclass: " << dsIt->first << ", queue size = " << dsIt->second.size() << endl;
        typename EventQueue::iterator eqIt = dsIt->second.begin();
        for ( ; eqIt != dsIt->second.end(); ++eqIt) {
            cout << "\t\tevent size = " << eqIt->size() << endl;
            typename EventToMix::iterator etmIt = eqIt->begin();
            for ( ; etmIt != eqIt->end(); ++etmIt) {
                cout << "\t\t\tpid: " << etmIt->first << ", list size = " << etmIt->second.size() << endl;
                typename ParticlesList::iterator plIt = etmIt->second.begin();
                for ( ; plIt != etmIt->second.end(); ++plIt) {
                    cout << "\t\t\t\tparticle charge: " << (*plIt)->getCharge() << endl;
                }
            }
        }
    }
    cout << "#####" << endl;
}

/*
 *
 * Because it's not possible to partially specialize the method buildPair, it's necessary, to define subclasses of specializations
 * of generic class HEventMixer
 *
 */

// Specialization for HParticlePair
class HPairEventMixer : public HEventMixer<HParticlePair, HParticlePair *>
{
    HParticlePair *buildPair(HParticlePair* pair1, HParticlePair* pair2) {
        HParticlePair *quad = new HParticlePair();
        quad->setPair(pair1,fPID1,pair2,fPID2,fMotherPID,0,fVertex);
        return quad;
    }
};

// Specialization for HParticleCand
class HParticleEventMixer : public HEventMixer<HParticleCand, HParticlePair>
{
    HParticlePair buildPair(HParticleCand* cand1, HParticleCand* cand2) {
        UInt_t flag = 0;
        HParticleTool::setPairFlags(flag,cand2,cand1);
        if(!fuse_kIsLepton) flag=flag|kNoUseRICH;
        HParticlePair pair;
        pair.setPair(cand1,fPID1,cand2,fPID2,fMotherPID,flag,fVertex);
        return pair;
    }
};

// General version (for TLorentzVector, HGeantKine, PParticle, ...)
template <class T>
class HGenericEventMixer : public HEventMixer<T,pair<T*, T*> >
{
    pair<T*,T*> buildPair(T* obj1, T* obj2) {
        return make_pair(obj1,obj2);    
    }
};

template <class T>
class HGenericEventMixerObj : public HEventMixer<T,pair<T, T> >
{
    pair<T,T> buildPair(T obj1, T obj2) {
        return make_pair(obj1,obj2);    
    }
};

#endif //HEVENTMIXER_H
