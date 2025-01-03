package gramfree

import (
    "math/rand"
    "time"
    //"os"
    //"fmt"
    "strings"
)

type Mutator struct {
    // corpus,num,len,graph
    Corpus []string
    Corpus_num int
    Corpus_len []int
    Corpus_graph []*Graph
    // current average_len = select corpus num
    Average_len int
    // mutator generate
    Testcase string
}

func (self *Mutator) Init() {
    self.Corpus = make([]string,0)
	self.Corpus_num = 0
	self.Corpus_len = make([]int,0)
	self.Corpus_graph = make([]*Graph,0)
	self.Average_len = 0 
	self.Testcase = ""
}

func (self *Mutator) Calculate_line(lines string,dbms string) int {
    var lines_len int
    switch dbms {
    case MongoDB,AgensGraph :
        // statements is sliced by ; 
        lines_len = strings.Count(lines,";")
    case Redis,KeyDB :
        // commands is sliced by \n
        lua_len := strings.Count(lines,"#!lua")
        lines_len = strings.Count(lines,"\n") - lua_len
    }
    return lines_len
}

func (self *Mutator) one_line(lines string,index int) string {
    slices := strings.SplitAfter(lines,";")
    line := slices[index]
    return line
}

func (self *Mutator) Corpus_add(lines string,length int){
    self.Corpus = append(self.Corpus,lines)
    self.Corpus_num ++
    self.Corpus_len = append(self.Corpus_len,length)
    average := self.Average_len
    if average != 0 {
        average += length
        self.Average_len = average / 2
    }else {
        self.Average_len = length
    }
    // trim Corpus
    if index := len(self.Corpus) - self.Average_len ; index > 0 {
        self.Corpus = self.Corpus[index:]
    }
}

func (self *Mutator) Corpus_last() string {
    if self.Corpus_num == 0 { return "" }
    return self.Corpus[self.Corpus_num -1]
}

func match(meta1,meta2 VertexP) (bool,map[string]string) {
    // replace metadata name
    replace := make(map[string]string,0)
    // different metadata type，match failed
    if meta1.V_meta.M_type != meta2.V_meta.M_type { return false,nil }
    // range its child meta
    for edge1 := meta1.V_out ; edge1 != nil ; edge1 = edge1.E_from_next {
        // keep edge meta->meta
        if edge1.E_type != ETYPE_m_to_m { continue }
        matched := false
        // every child in meta1 can be matched in meta2
        for edge2 := meta2.V_out ; edge2 != nil ; edge2 = edge2.E_from_next {
            if edge2.E_type != ETYPE_m_to_m { continue }
            if matched,rep := match(edge1.E_to,edge2.E_to) ; matched {
                // collect
                for key,value := range rep { 
                    replace[key] = value 
                } 
                break
            }
        }
        if matched == false { return false,nil }
    }
    // same metadata type , collect
    replace[meta1.V_meta.M_name] = meta2.V_meta.M_name
    return true,replace
}

func repair(stmt_used,stmt_touse VertexP,line string) bool {
    // replace metadata name
    replace := make(map[string]string,0)
    // parent metadata
    meta_touse := make([]VertexP,0)
    meta_used := make([]VertexP,0)
    // range edge to stmt_touse，must be etype_m_to_s
    for edge := stmt_touse.V_in ; edge != nil ; edge = edge.E_to_next {
        // parent metadata
        metadata := edge.E_from
        if metadata.V_meta.M_parent != "" { continue }
        meta_touse = append(meta_touse,metadata)
    }
    // range edge from stmt_used，must be etype_s_to_m
    for edge := stmt_used.V_out ; edge != nil ; edge = edge.E_from_next {
        // parent metadata
        metadata := edge.E_to
        if metadata.V_meta.M_parent != "" { continue }
        meta_used = append(meta_used,metadata)
    }
    // range match()
    for _,touse := range meta_touse {
        matched := false
        for _,used := range meta_used {
            if matched,rep := match(touse,used) ; matched {
                // collect
                for key,value := range rep { 
                    replace[key] = value 
                }
                break
            }
        }
        if matched == false { return false}
    }
    // replace key->value
    for key,value := range replace { 
        strings.Replace(line,key,value,-1) 
    }
    return true
}

// RPC
func (self *Mutator) Mutate(arg int,reply *int) error {
    rand.Seed(time.Now().Unix())
    corpus_num := self.Corpus_num
    average_len := self.Average_len
    var chosen string
    var offset,index int
    var graph *Graph
    //stmt tmp，used for repair()
    stmts := make([]VertexP,0)
    // generate testcase ,len = average_len
    for len := average_len ; len > 0 ; len -- {
        // cyclic corpus
        if average_len > corpus_num {
            offset = len % corpus_num 
            if offset == 0 { offset = corpus_num }
            offset --

        // choose from first
        }else {
            offset = 0
        }
        // chosen lines text
        chosen = self.Corpus[offset]
        //fmt.Println(offset,chosen)
        // lines's Graph
        graph = self.Corpus_graph[offset]
        //random choose one line from lines
    rand_choose:
        index = rand.Intn(self.Corpus_len[offset])
        line := self.one_line(chosen,index)
        line_stmt := graph.Stmts[index]
        // repair
        if line_stmt.V_in_num == 0 {
            // line has no parent
            goto repaired
        }else {
            // range find repairable
            ret := false
            for _,stmt := range stmts {
                ret = repair(stmt,line_stmt,line) 
                if ret == true { goto repaired }
            }
            // not repairable , choose again
            if ret == false { goto rand_choose }
        }
        // repairable
    repaired:
        self.Testcase += line
        stmts = append(stmts,line_stmt)
    }
    *reply = len(self.Testcase)
    return nil
}

// RPC
func (self *Mutator) Ret_testcase(arg int,reply *string) error {
    *reply = self.Testcase
    return nil
}
