// Generated from /Users/temper/Coding/Scale/Scale.g4 by ANTLR 4.13.1
import org.antlr.v4.runtime.atn.*;
import org.antlr.v4.runtime.dfa.DFA;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.*;
import org.antlr.v4.runtime.tree.*;
import java.util.List;
import java.util.Iterator;
import java.util.ArrayList;

@SuppressWarnings({"all", "warnings", "unchecked", "unused", "cast", "CheckReturnValue"})
public class ScaleParser extends Parser {
	static { RuntimeMetaData.checkVersion("4.13.1", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		T__0=1, T__1=2, T__2=3, T__3=4, T__4=5, T__5=6, T__6=7, T__7=8, T__8=9, 
		T__9=10, T__10=11, T__11=12, T__12=13, T__13=14, T__14=15, T__15=16, T__16=17, 
		T__17=18, T__18=19, T__19=20, T__20=21, T__21=22, T__22=23, T__23=24, 
		T__24=25, T__25=26, T__26=27, T__27=28, T__28=29, T__29=30, T__30=31, 
		T__31=32, T__32=33, T__33=34, T__34=35, ID=36, INT=37, FLOAT=38, STRING=39, 
		WS=40, COMMENT=41;
	public static final int
		RULE_root = 0, RULE_path = 1, RULE_template = 2, RULE_type = 3, RULE_parameter = 4, 
		RULE_function = 5, RULE_struct = 6, RULE_interface = 7, RULE_enum = 8, 
		RULE_union = 9, RULE_declaration = 10, RULE_definition = 11, RULE_block = 12, 
		RULE_expression = 13, RULE_literal = 14;
	private static String[] makeRuleNames() {
		return new String[] {
			"root", "path", "template", "type", "parameter", "function", "struct", 
			"interface", "enum", "union", "declaration", "definition", "block", "expression", 
			"literal"
		};
	}
	public static final String[] ruleNames = makeRuleNames();

	private static String[] makeLiteralNames() {
		return new String[] {
			null, "'::'", "'<'", "','", "'>'", "'lambda'", "'('", "')'", "':'", "'ref'", 
			"'const'", "'function'", "'end'", "'struct'", "'interface'", "'enum'", 
			"'['", "']'", "'='", "'union'", "'decl'", "'=>'", "'.'", "'++'", "'--'", 
			"'+'", "'-'", "'*'", "'/'", "'%'", "'=='", "'!='", "'<='", "'>='", "'true'", 
			"'false'"
		};
	}
	private static final String[] _LITERAL_NAMES = makeLiteralNames();
	private static String[] makeSymbolicNames() {
		return new String[] {
			null, null, null, null, null, null, null, null, null, null, null, null, 
			null, null, null, null, null, null, null, null, null, null, null, null, 
			null, null, null, null, null, null, null, null, null, null, null, null, 
			"ID", "INT", "FLOAT", "STRING", "WS", "COMMENT"
		};
	}
	private static final String[] _SYMBOLIC_NAMES = makeSymbolicNames();
	public static final Vocabulary VOCABULARY = new VocabularyImpl(_LITERAL_NAMES, _SYMBOLIC_NAMES);

	/**
	 * @deprecated Use {@link #VOCABULARY} instead.
	 */
	@Deprecated
	public static final String[] tokenNames;
	static {
		tokenNames = new String[_SYMBOLIC_NAMES.length];
		for (int i = 0; i < tokenNames.length; i++) {
			tokenNames[i] = VOCABULARY.getLiteralName(i);
			if (tokenNames[i] == null) {
				tokenNames[i] = VOCABULARY.getSymbolicName(i);
			}

			if (tokenNames[i] == null) {
				tokenNames[i] = "<INVALID>";
			}
		}
	}

	@Override
	@Deprecated
	public String[] getTokenNames() {
		return tokenNames;
	}

	@Override

	public Vocabulary getVocabulary() {
		return VOCABULARY;
	}

	@Override
	public String getGrammarFileName() { return "Scale.g4"; }

	@Override
	public String[] getRuleNames() { return ruleNames; }

	@Override
	public String getSerializedATN() { return _serializedATN; }

	@Override
	public ATN getATN() { return _ATN; }

	public ScaleParser(TokenStream input) {
		super(input);
		_interp = new ParserATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@SuppressWarnings("CheckReturnValue")
	public static class RootContext extends ParserRuleContext {
		public TerminalNode EOF() { return getToken(ScaleParser.EOF, 0); }
		public FunctionContext function() {
			return getRuleContext(FunctionContext.class,0);
		}
		public StructContext struct() {
			return getRuleContext(StructContext.class,0);
		}
		public InterfaceContext interface_() {
			return getRuleContext(InterfaceContext.class,0);
		}
		public EnumContext enum_() {
			return getRuleContext(EnumContext.class,0);
		}
		public UnionContext union() {
			return getRuleContext(UnionContext.class,0);
		}
		public DeclarationContext declaration() {
			return getRuleContext(DeclarationContext.class,0);
		}
		public DefinitionContext definition() {
			return getRuleContext(DefinitionContext.class,0);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public RootContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_root; }
	}

	public final RootContext root() throws RecognitionException {
		RootContext _localctx = new RootContext(_ctx, getState());
		enterRule(_localctx, 0, RULE_root);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(38);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case T__10:
				{
				setState(30);
				function();
				}
				break;
			case T__12:
				{
				setState(31);
				struct();
				}
				break;
			case T__13:
				{
				setState(32);
				interface_();
				}
				break;
			case T__14:
				{
				setState(33);
				enum_();
				}
				break;
			case T__18:
				{
				setState(34);
				union();
				}
				break;
			case T__19:
				{
				setState(35);
				declaration();
				}
				break;
			case T__20:
				{
				setState(36);
				definition();
				}
				break;
			case T__5:
				{
				setState(37);
				block();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			setState(40);
			match(EOF);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class PathContext extends ParserRuleContext {
		public List<TerminalNode> ID() { return getTokens(ScaleParser.ID); }
		public TerminalNode ID(int i) {
			return getToken(ScaleParser.ID, i);
		}
		public List<TemplateContext> template() {
			return getRuleContexts(TemplateContext.class);
		}
		public TemplateContext template(int i) {
			return getRuleContext(TemplateContext.class,i);
		}
		public PathContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_path; }
	}

	public final PathContext path() throws RecognitionException {
		PathContext _localctx = new PathContext(_ctx, getState());
		enterRule(_localctx, 2, RULE_path);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(42);
			match(ID);
			setState(44);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,1,_ctx) ) {
			case 1:
				{
				setState(43);
				template();
				}
				break;
			}
			setState(53);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==T__0) {
				{
				{
				setState(46);
				match(T__0);
				setState(47);
				match(ID);
				setState(49);
				_errHandler.sync(this);
				switch ( getInterpreter().adaptivePredict(_input,2,_ctx) ) {
				case 1:
					{
					setState(48);
					template();
					}
					break;
				}
				}
				}
				setState(55);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class TemplateContext extends ParserRuleContext {
		public List<ExpressionContext> expression() {
			return getRuleContexts(ExpressionContext.class);
		}
		public ExpressionContext expression(int i) {
			return getRuleContext(ExpressionContext.class,i);
		}
		public TemplateContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_template; }
	}

	public final TemplateContext template() throws RecognitionException {
		TemplateContext _localctx = new TemplateContext(_ctx, getState());
		enterRule(_localctx, 4, RULE_template);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(56);
			match(T__1);
			setState(57);
			expression();
			setState(62);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==T__2) {
				{
				{
				setState(58);
				match(T__2);
				setState(59);
				expression();
				}
				}
				setState(64);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(65);
			match(T__3);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class TypeContext extends ParserRuleContext {
		public PathContext path() {
			return getRuleContext(PathContext.class,0);
		}
		public TypeContext type() {
			return getRuleContext(TypeContext.class,0);
		}
		public List<ParameterContext> parameter() {
			return getRuleContexts(ParameterContext.class);
		}
		public ParameterContext parameter(int i) {
			return getRuleContext(ParameterContext.class,i);
		}
		public TypeContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_type; }
	}

	public final TypeContext type() throws RecognitionException {
		TypeContext _localctx = new TypeContext(_ctx, getState());
		enterRule(_localctx, 6, RULE_type);
		int _la;
		try {
			setState(87);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case ID:
				enterOuterAlt(_localctx, 1);
				{
				setState(67);
				path();
				}
				break;
			case T__4:
				enterOuterAlt(_localctx, 2);
				{
				setState(68);
				match(T__4);
				setState(69);
				match(T__5);
				setState(78);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==ID) {
					{
					setState(70);
					parameter();
					setState(75);
					_errHandler.sync(this);
					_la = _input.LA(1);
					while (_la==T__2) {
						{
						{
						setState(71);
						match(T__2);
						setState(72);
						parameter();
						}
						}
						setState(77);
						_errHandler.sync(this);
						_la = _input.LA(1);
					}
					}
				}

				setState(80);
				match(T__6);
				setState(81);
				match(T__7);
				setState(82);
				type();
				}
				break;
			case T__8:
				enterOuterAlt(_localctx, 3);
				{
				setState(83);
				match(T__8);
				setState(84);
				type();
				}
				break;
			case T__9:
				enterOuterAlt(_localctx, 4);
				{
				setState(85);
				match(T__9);
				setState(86);
				type();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ParameterContext extends ParserRuleContext {
		public TerminalNode ID() { return getToken(ScaleParser.ID, 0); }
		public TypeContext type() {
			return getRuleContext(TypeContext.class,0);
		}
		public ParameterContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_parameter; }
	}

	public final ParameterContext parameter() throws RecognitionException {
		ParameterContext _localctx = new ParameterContext(_ctx, getState());
		enterRule(_localctx, 8, RULE_parameter);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(89);
			match(ID);
			setState(90);
			match(T__7);
			setState(91);
			type();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class FunctionContext extends ParserRuleContext {
		public TerminalNode ID() { return getToken(ScaleParser.ID, 0); }
		public TypeContext type() {
			return getRuleContext(TypeContext.class,0);
		}
		public TemplateContext template() {
			return getRuleContext(TemplateContext.class,0);
		}
		public List<ParameterContext> parameter() {
			return getRuleContexts(ParameterContext.class);
		}
		public ParameterContext parameter(int i) {
			return getRuleContext(ParameterContext.class,i);
		}
		public List<ExpressionContext> expression() {
			return getRuleContexts(ExpressionContext.class);
		}
		public ExpressionContext expression(int i) {
			return getRuleContext(ExpressionContext.class,i);
		}
		public FunctionContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_function; }
	}

	public final FunctionContext function() throws RecognitionException {
		FunctionContext _localctx = new FunctionContext(_ctx, getState());
		enterRule(_localctx, 10, RULE_function);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(93);
			match(T__10);
			setState(94);
			match(ID);
			setState(96);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==T__1) {
				{
				setState(95);
				template();
				}
			}

			setState(98);
			match(T__5);
			setState(107);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==ID) {
				{
				setState(99);
				parameter();
				setState(104);
				_errHandler.sync(this);
				_la = _input.LA(1);
				while (_la==T__2) {
					{
					{
					setState(100);
					match(T__2);
					setState(101);
					parameter();
					}
					}
					setState(106);
					_errHandler.sync(this);
					_la = _input.LA(1);
				}
				}
			}

			setState(109);
			match(T__6);
			setState(110);
			match(T__7);
			setState(111);
			type();
			setState(115);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 1099507499092L) != 0)) {
				{
				{
				setState(112);
				expression();
				}
				}
				setState(117);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(118);
			match(T__11);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class StructContext extends ParserRuleContext {
		public TerminalNode ID() { return getToken(ScaleParser.ID, 0); }
		public TemplateContext template() {
			return getRuleContext(TemplateContext.class,0);
		}
		public List<DeclarationContext> declaration() {
			return getRuleContexts(DeclarationContext.class);
		}
		public DeclarationContext declaration(int i) {
			return getRuleContext(DeclarationContext.class,i);
		}
		public List<DefinitionContext> definition() {
			return getRuleContexts(DefinitionContext.class);
		}
		public DefinitionContext definition(int i) {
			return getRuleContext(DefinitionContext.class,i);
		}
		public List<FunctionContext> function() {
			return getRuleContexts(FunctionContext.class);
		}
		public FunctionContext function(int i) {
			return getRuleContext(FunctionContext.class,i);
		}
		public StructContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_struct; }
	}

	public final StructContext struct() throws RecognitionException {
		StructContext _localctx = new StructContext(_ctx, getState());
		enterRule(_localctx, 12, RULE_struct);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(120);
			match(T__12);
			setState(121);
			match(ID);
			setState(123);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==T__1) {
				{
				setState(122);
				template();
				}
			}

			setState(130);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 3147776L) != 0)) {
				{
				setState(128);
				_errHandler.sync(this);
				switch (_input.LA(1)) {
				case T__19:
					{
					setState(125);
					declaration();
					}
					break;
				case T__20:
					{
					setState(126);
					definition();
					}
					break;
				case T__10:
					{
					setState(127);
					function();
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(132);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(133);
			match(T__11);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class InterfaceContext extends ParserRuleContext {
		public TerminalNode ID() { return getToken(ScaleParser.ID, 0); }
		public TemplateContext template() {
			return getRuleContext(TemplateContext.class,0);
		}
		public List<FunctionContext> function() {
			return getRuleContexts(FunctionContext.class);
		}
		public FunctionContext function(int i) {
			return getRuleContext(FunctionContext.class,i);
		}
		public InterfaceContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_interface; }
	}

	public final InterfaceContext interface_() throws RecognitionException {
		InterfaceContext _localctx = new InterfaceContext(_ctx, getState());
		enterRule(_localctx, 14, RULE_interface);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(135);
			match(T__13);
			setState(136);
			match(ID);
			setState(138);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==T__1) {
				{
				setState(137);
				template();
				}
			}

			setState(143);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==T__10) {
				{
				{
				setState(140);
				function();
				}
				}
				setState(145);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(146);
			match(T__11);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class EnumContext extends ParserRuleContext {
		public List<TerminalNode> ID() { return getTokens(ScaleParser.ID); }
		public TerminalNode ID(int i) {
			return getToken(ScaleParser.ID, i);
		}
		public TemplateContext template() {
			return getRuleContext(TemplateContext.class,0);
		}
		public EnumContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_enum; }
	}

	public final EnumContext enum_() throws RecognitionException {
		EnumContext _localctx = new EnumContext(_ctx, getState());
		enterRule(_localctx, 16, RULE_enum);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(148);
			match(T__14);
			setState(149);
			match(ID);
			setState(151);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==T__1) {
				{
				setState(150);
				template();
				}
			}

			setState(162);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==T__15 || _la==ID) {
				{
				{
				setState(157);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==T__15) {
					{
					setState(153);
					match(T__15);
					setState(154);
					match(ID);
					setState(155);
					match(T__16);
					setState(156);
					match(T__17);
					}
				}

				setState(159);
				match(ID);
				}
				}
				setState(164);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(165);
			match(T__11);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class UnionContext extends ParserRuleContext {
		public TerminalNode ID() { return getToken(ScaleParser.ID, 0); }
		public TemplateContext template() {
			return getRuleContext(TemplateContext.class,0);
		}
		public List<DeclarationContext> declaration() {
			return getRuleContexts(DeclarationContext.class);
		}
		public DeclarationContext declaration(int i) {
			return getRuleContext(DeclarationContext.class,i);
		}
		public List<DefinitionContext> definition() {
			return getRuleContexts(DefinitionContext.class);
		}
		public DefinitionContext definition(int i) {
			return getRuleContext(DefinitionContext.class,i);
		}
		public UnionContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_union; }
	}

	public final UnionContext union() throws RecognitionException {
		UnionContext _localctx = new UnionContext(_ctx, getState());
		enterRule(_localctx, 18, RULE_union);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(167);
			match(T__18);
			setState(168);
			match(ID);
			setState(170);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==T__1) {
				{
				setState(169);
				template();
				}
			}

			setState(176);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==T__19 || _la==T__20) {
				{
				setState(174);
				_errHandler.sync(this);
				switch (_input.LA(1)) {
				case T__19:
					{
					setState(172);
					declaration();
					}
					break;
				case T__20:
					{
					setState(173);
					definition();
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(178);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(179);
			match(T__11);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class DeclarationContext extends ParserRuleContext {
		public TerminalNode ID() { return getToken(ScaleParser.ID, 0); }
		public TypeContext type() {
			return getRuleContext(TypeContext.class,0);
		}
		public DeclarationContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_declaration; }
	}

	public final DeclarationContext declaration() throws RecognitionException {
		DeclarationContext _localctx = new DeclarationContext(_ctx, getState());
		enterRule(_localctx, 20, RULE_declaration);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(181);
			match(T__19);
			setState(182);
			match(ID);
			setState(183);
			match(T__7);
			setState(184);
			type();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class DefinitionContext extends ParserRuleContext {
		public DeclarationContext declaration() {
			return getRuleContext(DeclarationContext.class,0);
		}
		public DefinitionContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_definition; }
	}

	public final DefinitionContext definition() throws RecognitionException {
		DefinitionContext _localctx = new DefinitionContext(_ctx, getState());
		enterRule(_localctx, 22, RULE_definition);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(186);
			match(T__20);
			setState(187);
			declaration();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class BlockContext extends ParserRuleContext {
		public List<ExpressionContext> expression() {
			return getRuleContexts(ExpressionContext.class);
		}
		public ExpressionContext expression(int i) {
			return getRuleContext(ExpressionContext.class,i);
		}
		public BlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_block; }
	}

	public final BlockContext block() throws RecognitionException {
		BlockContext _localctx = new BlockContext(_ctx, getState());
		enterRule(_localctx, 24, RULE_block);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(189);
			match(T__5);
			setState(193);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 1099507499092L) != 0)) {
				{
				{
				setState(190);
				expression();
				}
				}
				setState(195);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(196);
			match(T__6);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ExpressionContext extends ParserRuleContext {
		public TerminalNode ID() { return getToken(ScaleParser.ID, 0); }
		public LiteralContext literal() {
			return getRuleContext(LiteralContext.class,0);
		}
		public PathContext path() {
			return getRuleContext(PathContext.class,0);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public ExpressionContext expression() {
			return getRuleContext(ExpressionContext.class,0);
		}
		public ExpressionContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_expression; }
	}

	public final ExpressionContext expression() throws RecognitionException {
		ExpressionContext _localctx = new ExpressionContext(_ctx, getState());
		enterRule(_localctx, 26, RULE_expression);
		try {
			setState(221);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,24,_ctx) ) {
			case 1:
				enterOuterAlt(_localctx, 1);
				{
				setState(198);
				match(ID);
				}
				break;
			case 2:
				enterOuterAlt(_localctx, 2);
				{
				setState(199);
				literal();
				}
				break;
			case 3:
				enterOuterAlt(_localctx, 3);
				{
				setState(200);
				path();
				}
				break;
			case 4:
				enterOuterAlt(_localctx, 4);
				{
				setState(201);
				block();
				}
				break;
			case 5:
				enterOuterAlt(_localctx, 5);
				{
				setState(202);
				match(T__21);
				setState(203);
				match(ID);
				}
				break;
			case 6:
				enterOuterAlt(_localctx, 6);
				{
				setState(204);
				match(T__15);
				setState(205);
				expression();
				setState(206);
				match(T__16);
				}
				break;
			case 7:
				enterOuterAlt(_localctx, 7);
				{
				setState(208);
				match(T__22);
				}
				break;
			case 8:
				enterOuterAlt(_localctx, 8);
				{
				setState(209);
				match(T__23);
				}
				break;
			case 9:
				enterOuterAlt(_localctx, 9);
				{
				setState(210);
				match(T__24);
				}
				break;
			case 10:
				enterOuterAlt(_localctx, 10);
				{
				setState(211);
				match(T__25);
				}
				break;
			case 11:
				enterOuterAlt(_localctx, 11);
				{
				setState(212);
				match(T__26);
				}
				break;
			case 12:
				enterOuterAlt(_localctx, 12);
				{
				setState(213);
				match(T__27);
				}
				break;
			case 13:
				enterOuterAlt(_localctx, 13);
				{
				setState(214);
				match(T__28);
				}
				break;
			case 14:
				enterOuterAlt(_localctx, 14);
				{
				setState(215);
				match(T__29);
				}
				break;
			case 15:
				enterOuterAlt(_localctx, 15);
				{
				setState(216);
				match(T__30);
				}
				break;
			case 16:
				enterOuterAlt(_localctx, 16);
				{
				setState(217);
				match(T__1);
				}
				break;
			case 17:
				enterOuterAlt(_localctx, 17);
				{
				setState(218);
				match(T__31);
				}
				break;
			case 18:
				enterOuterAlt(_localctx, 18);
				{
				setState(219);
				match(T__3);
				}
				break;
			case 19:
				enterOuterAlt(_localctx, 19);
				{
				setState(220);
				match(T__32);
				}
				break;
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class LiteralContext extends ParserRuleContext {
		public TerminalNode INT() { return getToken(ScaleParser.INT, 0); }
		public TerminalNode FLOAT() { return getToken(ScaleParser.FLOAT, 0); }
		public TerminalNode STRING() { return getToken(ScaleParser.STRING, 0); }
		public LiteralContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_literal; }
	}

	public final LiteralContext literal() throws RecognitionException {
		LiteralContext _localctx = new LiteralContext(_ctx, getState());
		enterRule(_localctx, 28, RULE_literal);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(223);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 1013612281856L) != 0)) ) {
			_errHandler.recoverInline(this);
			}
			else {
				if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
				_errHandler.reportMatch(this);
				consume();
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	public static final String _serializedATN =
		"\u0004\u0001)\u00e2\u0002\u0000\u0007\u0000\u0002\u0001\u0007\u0001\u0002"+
		"\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002\u0004\u0007\u0004\u0002"+
		"\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002\u0007\u0007\u0007\u0002"+
		"\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002\u000b\u0007\u000b\u0002"+
		"\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e\u0001\u0000\u0001\u0000"+
		"\u0001\u0000\u0001\u0000\u0001\u0000\u0001\u0000\u0001\u0000\u0001\u0000"+
		"\u0003\u0000\'\b\u0000\u0001\u0000\u0001\u0000\u0001\u0001\u0001\u0001"+
		"\u0003\u0001-\b\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0003\u0001"+
		"2\b\u0001\u0005\u00014\b\u0001\n\u0001\f\u00017\t\u0001\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0005\u0002=\b\u0002\n\u0002\f\u0002@\t"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0003\u0001\u0003\u0001\u0003\u0001"+
		"\u0003\u0001\u0003\u0001\u0003\u0005\u0003J\b\u0003\n\u0003\f\u0003M\t"+
		"\u0003\u0003\u0003O\b\u0003\u0001\u0003\u0001\u0003\u0001\u0003\u0001"+
		"\u0003\u0001\u0003\u0001\u0003\u0001\u0003\u0003\u0003X\b\u0003\u0001"+
		"\u0004\u0001\u0004\u0001\u0004\u0001\u0004\u0001\u0005\u0001\u0005\u0001"+
		"\u0005\u0003\u0005a\b\u0005\u0001\u0005\u0001\u0005\u0001\u0005\u0001"+
		"\u0005\u0005\u0005g\b\u0005\n\u0005\f\u0005j\t\u0005\u0003\u0005l\b\u0005"+
		"\u0001\u0005\u0001\u0005\u0001\u0005\u0001\u0005\u0005\u0005r\b\u0005"+
		"\n\u0005\f\u0005u\t\u0005\u0001\u0005\u0001\u0005\u0001\u0006\u0001\u0006"+
		"\u0001\u0006\u0003\u0006|\b\u0006\u0001\u0006\u0001\u0006\u0001\u0006"+
		"\u0005\u0006\u0081\b\u0006\n\u0006\f\u0006\u0084\t\u0006\u0001\u0006\u0001"+
		"\u0006\u0001\u0007\u0001\u0007\u0001\u0007\u0003\u0007\u008b\b\u0007\u0001"+
		"\u0007\u0005\u0007\u008e\b\u0007\n\u0007\f\u0007\u0091\t\u0007\u0001\u0007"+
		"\u0001\u0007\u0001\b\u0001\b\u0001\b\u0003\b\u0098\b\b\u0001\b\u0001\b"+
		"\u0001\b\u0001\b\u0003\b\u009e\b\b\u0001\b\u0005\b\u00a1\b\b\n\b\f\b\u00a4"+
		"\t\b\u0001\b\u0001\b\u0001\t\u0001\t\u0001\t\u0003\t\u00ab\b\t\u0001\t"+
		"\u0001\t\u0005\t\u00af\b\t\n\t\f\t\u00b2\t\t\u0001\t\u0001\t\u0001\n\u0001"+
		"\n\u0001\n\u0001\n\u0001\n\u0001\u000b\u0001\u000b\u0001\u000b\u0001\f"+
		"\u0001\f\u0005\f\u00c0\b\f\n\f\f\f\u00c3\t\f\u0001\f\u0001\f\u0001\r\u0001"+
		"\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001"+
		"\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001"+
		"\r\u0001\r\u0001\r\u0001\r\u0003\r\u00de\b\r\u0001\u000e\u0001\u000e\u0001"+
		"\u000e\u0000\u0000\u000f\u0000\u0002\u0004\u0006\b\n\f\u000e\u0010\u0012"+
		"\u0014\u0016\u0018\u001a\u001c\u0000\u0001\u0002\u0000\"#%\'\u0105\u0000"+
		"&\u0001\u0000\u0000\u0000\u0002*\u0001\u0000\u0000\u0000\u00048\u0001"+
		"\u0000\u0000\u0000\u0006W\u0001\u0000\u0000\u0000\bY\u0001\u0000\u0000"+
		"\u0000\n]\u0001\u0000\u0000\u0000\fx\u0001\u0000\u0000\u0000\u000e\u0087"+
		"\u0001\u0000\u0000\u0000\u0010\u0094\u0001\u0000\u0000\u0000\u0012\u00a7"+
		"\u0001\u0000\u0000\u0000\u0014\u00b5\u0001\u0000\u0000\u0000\u0016\u00ba"+
		"\u0001\u0000\u0000\u0000\u0018\u00bd\u0001\u0000\u0000\u0000\u001a\u00dd"+
		"\u0001\u0000\u0000\u0000\u001c\u00df\u0001\u0000\u0000\u0000\u001e\'\u0003"+
		"\n\u0005\u0000\u001f\'\u0003\f\u0006\u0000 \'\u0003\u000e\u0007\u0000"+
		"!\'\u0003\u0010\b\u0000\"\'\u0003\u0012\t\u0000#\'\u0003\u0014\n\u0000"+
		"$\'\u0003\u0016\u000b\u0000%\'\u0003\u0018\f\u0000&\u001e\u0001\u0000"+
		"\u0000\u0000&\u001f\u0001\u0000\u0000\u0000& \u0001\u0000\u0000\u0000"+
		"&!\u0001\u0000\u0000\u0000&\"\u0001\u0000\u0000\u0000&#\u0001\u0000\u0000"+
		"\u0000&$\u0001\u0000\u0000\u0000&%\u0001\u0000\u0000\u0000\'(\u0001\u0000"+
		"\u0000\u0000()\u0005\u0000\u0000\u0001)\u0001\u0001\u0000\u0000\u0000"+
		"*,\u0005$\u0000\u0000+-\u0003\u0004\u0002\u0000,+\u0001\u0000\u0000\u0000"+
		",-\u0001\u0000\u0000\u0000-5\u0001\u0000\u0000\u0000./\u0005\u0001\u0000"+
		"\u0000/1\u0005$\u0000\u000002\u0003\u0004\u0002\u000010\u0001\u0000\u0000"+
		"\u000012\u0001\u0000\u0000\u000024\u0001\u0000\u0000\u00003.\u0001\u0000"+
		"\u0000\u000047\u0001\u0000\u0000\u000053\u0001\u0000\u0000\u000056\u0001"+
		"\u0000\u0000\u00006\u0003\u0001\u0000\u0000\u000075\u0001\u0000\u0000"+
		"\u000089\u0005\u0002\u0000\u00009>\u0003\u001a\r\u0000:;\u0005\u0003\u0000"+
		"\u0000;=\u0003\u001a\r\u0000<:\u0001\u0000\u0000\u0000=@\u0001\u0000\u0000"+
		"\u0000><\u0001\u0000\u0000\u0000>?\u0001\u0000\u0000\u0000?A\u0001\u0000"+
		"\u0000\u0000@>\u0001\u0000\u0000\u0000AB\u0005\u0004\u0000\u0000B\u0005"+
		"\u0001\u0000\u0000\u0000CX\u0003\u0002\u0001\u0000DE\u0005\u0005\u0000"+
		"\u0000EN\u0005\u0006\u0000\u0000FK\u0003\b\u0004\u0000GH\u0005\u0003\u0000"+
		"\u0000HJ\u0003\b\u0004\u0000IG\u0001\u0000\u0000\u0000JM\u0001\u0000\u0000"+
		"\u0000KI\u0001\u0000\u0000\u0000KL\u0001\u0000\u0000\u0000LO\u0001\u0000"+
		"\u0000\u0000MK\u0001\u0000\u0000\u0000NF\u0001\u0000\u0000\u0000NO\u0001"+
		"\u0000\u0000\u0000OP\u0001\u0000\u0000\u0000PQ\u0005\u0007\u0000\u0000"+
		"QR\u0005\b\u0000\u0000RX\u0003\u0006\u0003\u0000ST\u0005\t\u0000\u0000"+
		"TX\u0003\u0006\u0003\u0000UV\u0005\n\u0000\u0000VX\u0003\u0006\u0003\u0000"+
		"WC\u0001\u0000\u0000\u0000WD\u0001\u0000\u0000\u0000WS\u0001\u0000\u0000"+
		"\u0000WU\u0001\u0000\u0000\u0000X\u0007\u0001\u0000\u0000\u0000YZ\u0005"+
		"$\u0000\u0000Z[\u0005\b\u0000\u0000[\\\u0003\u0006\u0003\u0000\\\t\u0001"+
		"\u0000\u0000\u0000]^\u0005\u000b\u0000\u0000^`\u0005$\u0000\u0000_a\u0003"+
		"\u0004\u0002\u0000`_\u0001\u0000\u0000\u0000`a\u0001\u0000\u0000\u0000"+
		"ab\u0001\u0000\u0000\u0000bk\u0005\u0006\u0000\u0000ch\u0003\b\u0004\u0000"+
		"de\u0005\u0003\u0000\u0000eg\u0003\b\u0004\u0000fd\u0001\u0000\u0000\u0000"+
		"gj\u0001\u0000\u0000\u0000hf\u0001\u0000\u0000\u0000hi\u0001\u0000\u0000"+
		"\u0000il\u0001\u0000\u0000\u0000jh\u0001\u0000\u0000\u0000kc\u0001\u0000"+
		"\u0000\u0000kl\u0001\u0000\u0000\u0000lm\u0001\u0000\u0000\u0000mn\u0005"+
		"\u0007\u0000\u0000no\u0005\b\u0000\u0000os\u0003\u0006\u0003\u0000pr\u0003"+
		"\u001a\r\u0000qp\u0001\u0000\u0000\u0000ru\u0001\u0000\u0000\u0000sq\u0001"+
		"\u0000\u0000\u0000st\u0001\u0000\u0000\u0000tv\u0001\u0000\u0000\u0000"+
		"us\u0001\u0000\u0000\u0000vw\u0005\f\u0000\u0000w\u000b\u0001\u0000\u0000"+
		"\u0000xy\u0005\r\u0000\u0000y{\u0005$\u0000\u0000z|\u0003\u0004\u0002"+
		"\u0000{z\u0001\u0000\u0000\u0000{|\u0001\u0000\u0000\u0000|\u0082\u0001"+
		"\u0000\u0000\u0000}\u0081\u0003\u0014\n\u0000~\u0081\u0003\u0016\u000b"+
		"\u0000\u007f\u0081\u0003\n\u0005\u0000\u0080}\u0001\u0000\u0000\u0000"+
		"\u0080~\u0001\u0000\u0000\u0000\u0080\u007f\u0001\u0000\u0000\u0000\u0081"+
		"\u0084\u0001\u0000\u0000\u0000\u0082\u0080\u0001\u0000\u0000\u0000\u0082"+
		"\u0083\u0001\u0000\u0000\u0000\u0083\u0085\u0001\u0000\u0000\u0000\u0084"+
		"\u0082\u0001\u0000\u0000\u0000\u0085\u0086\u0005\f\u0000\u0000\u0086\r"+
		"\u0001\u0000\u0000\u0000\u0087\u0088\u0005\u000e\u0000\u0000\u0088\u008a"+
		"\u0005$\u0000\u0000\u0089\u008b\u0003\u0004\u0002\u0000\u008a\u0089\u0001"+
		"\u0000\u0000\u0000\u008a\u008b\u0001\u0000\u0000\u0000\u008b\u008f\u0001"+
		"\u0000\u0000\u0000\u008c\u008e\u0003\n\u0005\u0000\u008d\u008c\u0001\u0000"+
		"\u0000\u0000\u008e\u0091\u0001\u0000\u0000\u0000\u008f\u008d\u0001\u0000"+
		"\u0000\u0000\u008f\u0090\u0001\u0000\u0000\u0000\u0090\u0092\u0001\u0000"+
		"\u0000\u0000\u0091\u008f\u0001\u0000\u0000\u0000\u0092\u0093\u0005\f\u0000"+
		"\u0000\u0093\u000f\u0001\u0000\u0000\u0000\u0094\u0095\u0005\u000f\u0000"+
		"\u0000\u0095\u0097\u0005$\u0000\u0000\u0096\u0098\u0003\u0004\u0002\u0000"+
		"\u0097\u0096\u0001\u0000\u0000\u0000\u0097\u0098\u0001\u0000\u0000\u0000"+
		"\u0098\u00a2\u0001\u0000\u0000\u0000\u0099\u009a\u0005\u0010\u0000\u0000"+
		"\u009a\u009b\u0005$\u0000\u0000\u009b\u009c\u0005\u0011\u0000\u0000\u009c"+
		"\u009e\u0005\u0012\u0000\u0000\u009d\u0099\u0001\u0000\u0000\u0000\u009d"+
		"\u009e\u0001\u0000\u0000\u0000\u009e\u009f\u0001\u0000\u0000\u0000\u009f"+
		"\u00a1\u0005$\u0000\u0000\u00a0\u009d\u0001\u0000\u0000\u0000\u00a1\u00a4"+
		"\u0001\u0000\u0000\u0000\u00a2\u00a0\u0001\u0000\u0000\u0000\u00a2\u00a3"+
		"\u0001\u0000\u0000\u0000\u00a3\u00a5\u0001\u0000\u0000\u0000\u00a4\u00a2"+
		"\u0001\u0000\u0000\u0000\u00a5\u00a6\u0005\f\u0000\u0000\u00a6\u0011\u0001"+
		"\u0000\u0000\u0000\u00a7\u00a8\u0005\u0013\u0000\u0000\u00a8\u00aa\u0005"+
		"$\u0000\u0000\u00a9\u00ab\u0003\u0004\u0002\u0000\u00aa\u00a9\u0001\u0000"+
		"\u0000\u0000\u00aa\u00ab\u0001\u0000\u0000\u0000\u00ab\u00b0\u0001\u0000"+
		"\u0000\u0000\u00ac\u00af\u0003\u0014\n\u0000\u00ad\u00af\u0003\u0016\u000b"+
		"\u0000\u00ae\u00ac\u0001\u0000\u0000\u0000\u00ae\u00ad\u0001\u0000\u0000"+
		"\u0000\u00af\u00b2\u0001\u0000\u0000\u0000\u00b0\u00ae\u0001\u0000\u0000"+
		"\u0000\u00b0\u00b1\u0001\u0000\u0000\u0000\u00b1\u00b3\u0001\u0000\u0000"+
		"\u0000\u00b2\u00b0\u0001\u0000\u0000\u0000\u00b3\u00b4\u0005\f\u0000\u0000"+
		"\u00b4\u0013\u0001\u0000\u0000\u0000\u00b5\u00b6\u0005\u0014\u0000\u0000"+
		"\u00b6\u00b7\u0005$\u0000\u0000\u00b7\u00b8\u0005\b\u0000\u0000\u00b8"+
		"\u00b9\u0003\u0006\u0003\u0000\u00b9\u0015\u0001\u0000\u0000\u0000\u00ba"+
		"\u00bb\u0005\u0015\u0000\u0000\u00bb\u00bc\u0003\u0014\n\u0000\u00bc\u0017"+
		"\u0001\u0000\u0000\u0000\u00bd\u00c1\u0005\u0006\u0000\u0000\u00be\u00c0"+
		"\u0003\u001a\r\u0000\u00bf\u00be\u0001\u0000\u0000\u0000\u00c0\u00c3\u0001"+
		"\u0000\u0000\u0000\u00c1\u00bf\u0001\u0000\u0000\u0000\u00c1\u00c2\u0001"+
		"\u0000\u0000\u0000\u00c2\u00c4\u0001\u0000\u0000\u0000\u00c3\u00c1\u0001"+
		"\u0000\u0000\u0000\u00c4\u00c5\u0005\u0007\u0000\u0000\u00c5\u0019\u0001"+
		"\u0000\u0000\u0000\u00c6\u00de\u0005$\u0000\u0000\u00c7\u00de\u0003\u001c"+
		"\u000e\u0000\u00c8\u00de\u0003\u0002\u0001\u0000\u00c9\u00de\u0003\u0018"+
		"\f\u0000\u00ca\u00cb\u0005\u0016\u0000\u0000\u00cb\u00de\u0005$\u0000"+
		"\u0000\u00cc\u00cd\u0005\u0010\u0000\u0000\u00cd\u00ce\u0003\u001a\r\u0000"+
		"\u00ce\u00cf\u0005\u0011\u0000\u0000\u00cf\u00de\u0001\u0000\u0000\u0000"+
		"\u00d0\u00de\u0005\u0017\u0000\u0000\u00d1\u00de\u0005\u0018\u0000\u0000"+
		"\u00d2\u00de\u0005\u0019\u0000\u0000\u00d3\u00de\u0005\u001a\u0000\u0000"+
		"\u00d4\u00de\u0005\u001b\u0000\u0000\u00d5\u00de\u0005\u001c\u0000\u0000"+
		"\u00d6\u00de\u0005\u001d\u0000\u0000\u00d7\u00de\u0005\u001e\u0000\u0000"+
		"\u00d8\u00de\u0005\u001f\u0000\u0000\u00d9\u00de\u0005\u0002\u0000\u0000"+
		"\u00da\u00de\u0005 \u0000\u0000\u00db\u00de\u0005\u0004\u0000\u0000\u00dc"+
		"\u00de\u0005!\u0000\u0000\u00dd\u00c6\u0001\u0000\u0000\u0000\u00dd\u00c7"+
		"\u0001\u0000\u0000\u0000\u00dd\u00c8\u0001\u0000\u0000\u0000\u00dd\u00c9"+
		"\u0001\u0000\u0000\u0000\u00dd\u00ca\u0001\u0000\u0000\u0000\u00dd\u00cc"+
		"\u0001\u0000\u0000\u0000\u00dd\u00d0\u0001\u0000\u0000\u0000\u00dd\u00d1"+
		"\u0001\u0000\u0000\u0000\u00dd\u00d2\u0001\u0000\u0000\u0000\u00dd\u00d3"+
		"\u0001\u0000\u0000\u0000\u00dd\u00d4\u0001\u0000\u0000\u0000\u00dd\u00d5"+
		"\u0001\u0000\u0000\u0000\u00dd\u00d6\u0001\u0000\u0000\u0000\u00dd\u00d7"+
		"\u0001\u0000\u0000\u0000\u00dd\u00d8\u0001\u0000\u0000\u0000\u00dd\u00d9"+
		"\u0001\u0000\u0000\u0000\u00dd\u00da\u0001\u0000\u0000\u0000\u00dd\u00db"+
		"\u0001\u0000\u0000\u0000\u00dd\u00dc\u0001\u0000\u0000\u0000\u00de\u001b"+
		"\u0001\u0000\u0000\u0000\u00df\u00e0\u0007\u0000\u0000\u0000\u00e0\u001d"+
		"\u0001\u0000\u0000\u0000\u0019&,15>KNW`hks{\u0080\u0082\u008a\u008f\u0097"+
		"\u009d\u00a2\u00aa\u00ae\u00b0\u00c1\u00dd";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}