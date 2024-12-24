package gramfree

import (
    "regexp"
)

/* types */
type VertexType uint8
type EdgeType uint8
type MetaType uint8
type MetaStatus uint8
type MetadataP *Metadata
type VertexP *Vertex
type EdgeP *Edge
type MetamaP *Metamap

const(
    VTYPE_metadata VertexType = iota
    VTYPE_statement 
)

const(
    ETYPE_s_to_m EdgeType = iota
    ETYPE_m_to_s 
    ETYPE_m_to_m 
)
    
const(
    // redis types
    MTYPE_hash = iota
    MTYPE_str
    MTYPE_list
    MTYPE_set
    MTYPE_zset
    MTYPE_stream
    MTYPE_geo
    // default
    MTYPE_default
)

const(
    MSTAT_create MetaStatus = iota
    MSTAT_drop 
    MSTAT_keep 
)
    
/* structs */
type Metadata struct{
    M_type MetaType
    /* different table can have same column name */
    M_parent string
    M_name string
}

type Vertex struct{
    V_type VertexType
    // vertex data
    V_meta MetadataP
    V_stmt string
    V_in,V_out EdgeP
    V_in_num,V_out_num int
}

type Edge struct{
    E_type EdgeType
    E_from VertexP
    E_to VertexP
    /* from the same vertex */
    E_from_next EdgeP
    /* to the same vertex */
    E_to_next EdgeP
}

type Metamap struct{
    /* different table can have same column name  */
    /* double map: [name,parent] -> int (slice index) */
    Old_map map[string]map[string]int
    New_map map[string]map[string]int
}

type Graph struct{
    Meta_num,Stmt_num int
    Metas []VertexP
    Stmts []VertexP
    Map MetamaP
}

/*  Metamap methods */
func (self *Metamap) Update(){
    // empty Old_map
    self.Old_map = make(map[string]map[string]int)
    // copy New -> Old
    for k1,v1 := range self.New_map {
        for k2,v2 := range v1 {
            inner := make(map[string]int)
            inner[k2] = v2
            self.Old_map[k1] = inner
        }
    }
    //empty New_map
    self.New_map = make(map[string]map[string]int)
}

func (self *Metamap) check_situation() (MetaStatus,[]int) {
    var status MetaStatus
    status = MSTAT_keep
    m_index_slice := make([]int,0)
    /* key is deleted */
    for key1,inner := range self.Old_map {
        for key2,value := range inner {
            /* old_map has but new_map don't */
            if _,contain := self.New_map[key1][key2] ; !contain {
                if status != MSTAT_drop { status = MSTAT_drop }
                m_index_slice = append(m_index_slice,value)
            }
        }
    }
    if status == MSTAT_drop { return status,m_index_slice }
    /* key is created */
    for key1,inner := range self.New_map {
        for key2,value := range inner {
            /* new_map has but old_map don't */
            if _,contain := self.Old_map[key1][key2] ; !contain {
                if status != MSTAT_create { status = MSTAT_create }
                m_index_slice = append(m_index_slice,value)
            }
        }
    }
    if status == MSTAT_create { return status,m_index_slice }
    /* key is keeped */
    return status,m_index_slice
}

/*  Graph  methods */

func (self *Graph) add_vertex_stmt(vertex string){
    stmt := new(Vertex)
    stmt.V_type = VTYPE_statement
    stmt.V_stmt = vertex
    self.Stmts = append(self.Stmts,stmt)
    self.Stmt_num ++
}

func (self *Graph) add_vertex_meta(vertex MetadataP){
    meta := new(Vertex)
    meta.V_type = VTYPE_metadata
    meta.V_meta = vertex
    self.Metas = append(self.Metas,meta)
    self.Meta_num ++
}

func (self *Graph) add_edge(e_type EdgeType,index_from,index_to int){
    edge := new(Edge)
    edge.E_type = e_type
    switch e_type{
        case ETYPE_s_to_m :
            stmt := self.Stmts[index_from]
            meta := self.Metas[index_to]
            edge.E_from = stmt
            edge.E_to = meta
            edge.E_from_next = stmt.V_out
            edge.E_to_next = meta.V_in
            stmt.V_out = edge
            stmt.V_out_num ++
            meta.V_in = edge
            meta.V_in_num ++
        case ETYPE_m_to_s :
            meta := self.Metas[index_from]
            stmt := self.Stmts[index_to]
            edge.E_from = meta
            edge.E_to = stmt
            edge.E_from_next = meta.V_out
            edge.E_to_next = stmt.V_in
            meta.V_out = edge
            meta.V_out_num ++
            stmt.V_in = edge
            stmt.V_in_num ++
        case ETYPE_m_to_m :
            meta_from := self.Metas[index_from]
            meta_to := self.Metas[index_to]
            edge.E_from = meta_from
            edge.E_to = meta_to
            edge.E_from_next = meta_from.V_out
            edge.E_to_next = meta_to.V_in
            meta_from.V_out = edge
            meta_from.V_out_num ++
            meta_to.V_in = edge
            meta_to.V_in_num ++  
        }
    }

    /*
    1.metadata has three situation: create, delete,keep
    2.different table can have same column name
    */
func (self *Graph) Build_graph(stmtstr string){
    /* Step 1 : copy graph、add stmtstr */
    graph_copy := new(Graph)
    *graph_copy = *self
    graph_copy.add_vertex_stmt(stmtstr)
    /* Step 2 :，add edge old_metadate -> stmtstr (ETYPE_m_to_s) */
    s_index := graph_copy.Stmt_num -1
    var reg *regexp.Regexp
    for m_index,meta := range graph_copy.Metas {
        // regular match
        reg = regexp.MustCompile(meta.V_meta.M_name + "[ |;|,|)]")
        if reg.MatchString(stmtstr) {
            reg = regexp.MustCompile(meta.V_meta.M_parent + "[ |;|,|)]")
            // different table can have same column name
            if reg.MatchString(stmtstr) {
                graph_copy.add_edge(ETYPE_m_to_s,m_index,s_index)
            }
        }
    }
    /* Step 3 : add edge depends on three situation */
    // Metamap
    Map := *self.Map 
    status,m_index_slice := Map.check_situation()
    slice_len := len(m_index_slice)
    hitmap := make([]bool,slice_len)
    switch status {
    case MSTAT_create :
        /* add new metadata ，add edge ETYPE_s_to_m、ETYPE_m_to_m */
        /* start from metadata has no parent  */
        for hit,m_index := range m_index_slice {
            meta := graph_copy.Metas[m_index].V_meta
            if meta.M_parent == "" {
                graph_copy.add_edge(ETYPE_s_to_m,s_index,m_index)
                hitmap[hit] = true
                /* deal with its child metadata */
                for hit_child,m_child_index := range m_index_slice {
                    metas := graph_copy.Metas
                    if metas[m_index].V_meta.M_name == metas[m_child_index].V_meta.M_parent {
                        graph_copy.add_edge(ETYPE_s_to_m,s_index,m_child_index)
                        graph_copy.add_edge(ETYPE_m_to_m,m_index,m_child_index)
                        hitmap[hit_child] = true
                    }
                }
            }
        }
        /* other metadata，should be child of old metadata */
        for hit,m_index := range m_index_slice {
            if hitmap[hit] == false {
                meta_name :=graph_copy.Metas[m_index].V_meta.M_name
                meta_pname := graph_copy.Metas[m_index].V_meta.M_parent
                /* assert */
                assert := false
                for next := graph_copy.Stmts[s_index].V_in ; next != nil ; next = next.E_to_next {
                    if meta_pname == next.E_from.V_meta.M_name {
                        assert = true
                        break
                    }
                }
                if !assert { panic(meta_name) }
            }
        }
    case MSTAT_drop :
        // droped is related to old metadata
        for _,m_index := range m_index_slice {
            meta_name := graph_copy.Metas[m_index].V_meta.M_name
            /* assert */
            assert := false
            for next1 := graph_copy.Stmts[s_index].V_in ; next1 != nil ; next1 = next1.E_to_next {
                // for example : drop a_pkey but not has edge a_pkey -> DROP a
                for next2 := next1.E_from.V_in ; next2 != nil ; next2 = next2.E_to_next {
                    if next2.E_from.V_type != VTYPE_statement { continue }
                    for next3 := next2.E_from.V_out ; next3 != nil ; next3 = next3.E_from_next {
                        if meta_name == next3.E_to.V_meta.M_name {
                            assert = true
                            break
                        }
                    }
                }    
            }
            if !assert { panic(meta_name) }
        }
    case MSTAT_keep :
    /* do nothing */
    }
    /* Step 4 : update graph */
    self.Metas=graph_copy.Metas;
    self.Stmts=graph_copy.Stmts;
}

/* build_metamap before build_graph */
func (self *Graph) Build_metamap(nm string,tp string,pnm string){
    var mtp MetaType
    switch tp {
    // redis type
    case "STR" :
        mtp = MTYPE_str
    case "HASH" :
        mtp = MTYPE_hash
    case "LIST" :
        mtp = MTYPE_list
    case "SET" :
        mtp = MTYPE_set
    case "ZSET" :
        mtp = MTYPE_zset
    case "STREAM" :
        mtp = MTYPE_stream
    case "GEO" :
        mtp = MTYPE_geo
    // default
    default :
        mtp = MTYPE_default
    }
    for index,meta := range self.Metas {
        if meta.V_meta.M_name == nm && meta.V_meta.M_parent == pnm && meta.V_meta.M_type == mtp {
            inner := make(map[string]int)
            inner[pnm] = index
            self.Map.New_map[nm] = inner
            return
        }
    }
    data := new(Metadata)
    data.M_name = nm
    data.M_type = mtp
    data.M_parent = pnm
    self.add_vertex_meta(data)
    inner := make(map[string]int)
    inner[pnm] = self.Meta_num -1
    self.Map.New_map[nm] = inner
}
