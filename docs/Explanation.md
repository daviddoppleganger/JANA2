---
title: JANA: Multi-threaded HENP Event Reconstruction
---

<center>
<table border="0" width="100%" align="center">
<TH width="20%"><A href="index.html">Welcome</A></TH>
<TH width="20%"><A href="Tutorial.html">Tutorial</A></TH>
<TH width="20%"><A href="Howto.html">How-to guides</A></TH>
<TH width="20%"><A href="Explanation.html">Principles</A></TH>
<TH width="20%"><A href="Reference.html">Reference</A></TH>
</table>
</center>

Principles
===========

This section provides higher-level background and context for JANA, and discusses JANA's design philosophy and the
associated tradeoffs.

## JANA concepts

- JObjects are data containers for specific resuts, e.g. clusters or tracks. They may be plain-old structs or they may
optionally inherit from (e.g.) ROOT or NumPy datatypes. 

- JEventSources take a file or messaging producer which provides raw event data, and exposes it to JANA as a stream.

- JFactories calculate a specific result on an event-by-event basis. Their inputs may come from an EventSource or may
be computed via other JFactories. All results are evaluated lazily and cached until the entire event is finished processing.
in order to do so. Importantly, JFactories are decoupled from one another via the JEvent interface. It should make no
difference to the JFactory where its input data came from, as long as it has the correct type and tag. While the [Factory 
Pattern](https://en.wikipedia.org/wiki/Factory_method_pattern) usually abstracts away the _subtype_ of the class being 
created, in our case it abstracts away the _number of instances_ created instead. For instance, a ClusterFactory may 
take m Hit objects and produce n Cluster objects, where m and n vary per event and won't be known until that
 event gets processed. 

- JEventProcessors run desired JFactories over the event stream and write the results to an output file or messaging
consumer. JFactories form a lazy directed acyclic graph, whereas JEventProcessors trigger their actual evaluation. 



## Design philosophy

JANA's design philosophy can be boiled down to five values, ordered by importance:

1. Simple to use

JANA chooses its battles carefully. First and foremost, JANA is about parallelizing computations over data organized
into events. From a 30000-foot view, it should look more like OpenMP or Thread Building Blocks or RaftLib than like ROOT. 
Unlike the aforementioned, JANA's vocabulary of abstractions is designed around the needs of physicists rather than 
general programmers. However, JANA does not attempt to meet _all_ of the needs of physicists.

JANA recognizes when coinciding concerns ought to be handled orthogonally. A good example is persistence. JANA does not
seek to provide its own persistence layer, nor does it require the user to commit to a specific dependency such as ROOT
or Numpy or Apache Arrow. Instead, JANA tries to make it feasible for the user to choose their persistence layer independently.
This way, if a collaboration decides they wish to (for instance) migrate from ROOT to Arrow, they have a well-defined migration
path which keeps the core analysis code largely intact.

In particular, this means minimizing the complexity of the build system and minimizing orchestration. Building code
against JANA should require nothing more than implementing certain key interfaces, adding a single path to includes,
and linking against a single library. 

2. Well-organized

While JANA's primary goal is running code in parallel, its secondary goal is imposing an organizing principle on
the users' codebase. This can be invaluable in a large collaboration where members vary in programming skill. Specifically, 
JANA organizes processing logic into decoupled units. JFactories are agnostic of how and when their prerequisites are 
computed, are only run when actually needed, and cache their results for reuse. Different analyses can coexist in separate
JEventProcessors. Components can be compiled into independent plugins, to be mixed and matched at runtime. All together, 
JANA enforces an organizing principle that enables groups to develop and test their code with both freedom and discipline.


3. Safe

JANA recognizes that not all of its users are proficient parallel programmers, and it steers users towards patterns which
mitigate some of the pitfalls. Specifically, it provides:

- **Modern C++ features** such as smart pointers and judicious templating, to discourage common classes of bugs. JANA seeks to
make its memory ownership semantics explicit in the type system as much as possible.

- **Internally managed locks** to reduce the learning curve and discourage tricky parallelism bugs.

- **A stable API** with an effort towards backwards-compatibility, so that everybody can benefit from new features
and performance/stability improvements.


4. Fast

JANA uses low-level optimizations wherever it can in order to boost performance. 

5. Flexible

The simplest use case for JANA is to read a file of batched events, process each event independently, and aggregate 
the results into a new file. However, it can be used in more sophisticated ways. 

- Disentangling: Input data is bundled into blocks (each containing an array of entangled events) and we want to 
parse each block in order to emit a stream of events (_flatmap_)

- Software triggers: With streaming data readout, we may want to accept a stream of raw hit data and let JANA 
determine the event boundaries. Arbitrary triggers can be created using existing JFactories. (_windowed join_)

- Subevent-level parallelism: This is necessary if individual events are very large. It may also play a role in 
effectively utilizing a GPU, particularly as machine learning is adopted in reconstruction (_flatmap+merge_)

JANA is also flexible enough to be compiled and run different ways. Users may compile their code into a standalone 
executable, into one or more plugins which can be run by a generic executable, or run from a Jupyter notebook. 


## Comparison to other frameworks

Many different event reconstruction frameworks exist. The following are frequently compared and contrasted with JANA:

- [Clara](https://claraweb.jlab.org/clara/) While JANA specializes in thread-level parallelism, Clara
 uses node-level parallelism via a message-passing interface. This higher level of abstraction comes with some performance
 overhead and significant orchestration requirements. On the other hand, it can scale to larger problem sizes and 
 support more general stream topologies. JANA is to OpenMP as Clara is to MPI.

- [Fun4all](https://github.com/sPHENIX-Collaboration/Fun4All) Comparison coming soon!

